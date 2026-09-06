#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Unit/RTAnimCatalogTypes.h"
#include "RTAnimCatalogLibrary.generated.h"

/**
 * Lettura, scrittura, allocazione degli ID e validazione del catalogo delle animazioni.
 *
 * Funzioni pure e deterministiche: nessun Actor, nessun mondo, nessun `DeltaTime`. Testabili headless, come
 * `URTIconLibrary` e `URTPresentationBindingLibrary` — e per la stessa ragione: **un contratto rotto si scopre
 * in CI e non a schermo**.
 *
 * ⚠️ **Il validator e' diviso in due meta', e la divisione non e' estetica.** `ValidateCatalog` gira ovunque
 * perche' non tocca il disco; `ValidateReferents` richiede i pack Paragon (`.gitignore:105`, ~48 GB) e su una
 * macchina che non li ha deve dire `NOT RUN`, non restituire un array vuoto che sembra un successo.
 *
 * 🔴 **IL VERSO DEL FLUSSO, e perche' due dati nominano le stesse clip senza essere in conflitto** (#2442).
 *
 *     Data/Anim/AnimCatalog.json        memoria del GIUDIZIO umano — authoring
 *             |                          chi esiste, chi e' stata guardata, cosa si e' deciso
 *             |  bind di una Promoted    (#2443)
 *             v
 *     URTUnitAnimInstance::ClipsPerHero  cio' che il gioco SUONA — runtime
 *             |
 *             v
 *     gate di cook                       legge SOLO il CDO
 *
 * ⛔ **Il runtime non legge mai questo catalogo, e non e' una preferenza di stile.** Un JSON sotto `Data/`
 * non e' un asset versionato sotto `/Game/RT`, quindi non e' un referente che il cook sappia seguire
 * (`D-262`): farlo leggere a runtime metterebbe la presentazione su una via che il pacchetto non porta, e
 * il difetto si vedrebbe solo su packaged, come posa di riferimento.
 *
 * ⛔ Per la stessa ragione il gate `RefactorTactics.Packaging.RequiredAnimationClipsAreCooked` interroga il
 * **CDO** e non questo file: cio' che va cotto e' cio' che suona, non cio' che e' stato catalogato.
 */
UCLASS()
class REFACTORTACTICS_API URTAnimCatalogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Prefisso obbligatorio di ogni `AV_ID`. */
	static const TCHAR* IdPrefix;

	/** Percorso relativo dell'authoring source dentro il progetto. */
	static const TCHAR* CatalogRelativePath;

	/**
	 * Percorso canonico dell'authoring source: `<ProjectDir>/Data/Anim/AnimCatalog.json`.
	 *
	 * ⚠️ `Data/` **non** e' in `DirectoriesToAlwaysStageAsUFS`: l'authoring source non entra nel pacchetto. In
	 * v0.1 il suo consumatore e' una persona e il tooling, non il runtime.
	 */
	static FString DefaultCatalogPath();

	// -----------------------------------------------------------------------------------------------------
	// ID
	// -----------------------------------------------------------------------------------------------------

	/** `6` -> `AV_0006`. Oltre `9999` la larghezza cresce: `AV_10000`. Nessun wraparound. */
	static FString MakeId(int32 Number);

	/**
	 * `AV_0006` -> `6`. Falso se la stringa non e' `AV_` seguito da **almeno una cifra e nient'altro**.
	 *
	 * ⚠️ Il confronto fra ID e' **numerico sul suffisso**, mai lessicale sulla stringa: lessicalmente
	 * `AV_10000` viene prima di `AV_9999`, e un high-water mark confrontato cosi' comincerebbe a riciclare
	 * esattamente al superamento delle quattro cifre.
	 */
	static bool ParseId(const FName& Id, int32& OutNumber);

	/**
	 * Aggiunge una voce `Unreviewed` per ogni path non ancora presente, e conia il suo `AV_ID`.
	 *
	 * 🔴 **E' l'UNICA funzione che conia un `AV_ID`**, in tutto il progetto. L'ID successivo viene da
	 * `Catalog.NextId` — l'high-water mark persistito — e **mai** da `max(id esistenti) + 1`: questa funzione
	 * non guarda nemmeno gli ID gia' assegnati, ed e' cosi' che non puo' riciclarne uno.
	 *
	 * ⛔ Scrive **solo** `Unreviewed`, e non tocca `Authored` delle voci esistenti. Non deduce `AssetName`,
	 * `Label` o `Status` dal nome del file: dedurre produrrebbe un dato che sembra misurato e non lo e'.
	 *
	 * Idempotente sui path gia' a catalogo: richiamarla due volte non duplica nulla e non consuma ID.
	 *
	 * @return quante voci sono state aggiunte.
	 */
	static int32 AllocateIds(FRTAnimCatalog& Catalog, const TArray<FString>& NewAssetPaths);

	// -----------------------------------------------------------------------------------------------------
	// Lettura / scrittura
	// -----------------------------------------------------------------------------------------------------

	/**
	 * Interpreta il testo JSON dell'authoring source.
	 *
	 * 🔴 **Non conia ID, non ripara, non promuove.** Una voce senza `id` e' un errore, non una voce da
	 * riparare al volo: un loader che auto-assegna e' precisamente il modo in cui un ID si ricicla senza che
	 * nessuno lo veda. Gli ID escono **come stanno scritti nel file**, nell'ordine del file.
	 *
	 * @return true se il testo e' leggibile. Altrimenti `OutError` dice **cosa** non va, in una riga leggibile
	 *         da chi ha scritto il file.
	 */
	static bool LoadFromString(const FString& JsonText, FRTAnimCatalog& OutCatalog, FString& OutError);

	/** Come sopra, dal disco. `OutError` distingue «file assente» da «file illeggibile». */
	static bool LoadFromFile(const FString& FilePath, FRTAnimCatalog& OutCatalog, FString& OutError);

	/**
	 * Serializza il catalogo.
	 *
	 * ⚠️ Le chiavi escono in un **ordine fisso scritto a mano**, non nell'ordine di iterazione di un
	 * `FJsonObject` — che e' una `TMap`, e la cui iterazione non e' garantita. Un authoring source diffabile
	 * che riordina le proprie chiavi a ogni salvataggio produrrebbe diff di puro rumore, e il primo effetto
	 * sarebbe che nessuno li legge piu'.
	 */
	static bool SaveToString(const FRTAnimCatalog& Catalog, FString& OutJsonText);

	// -----------------------------------------------------------------------------------------------------
	// Validazione
	// -----------------------------------------------------------------------------------------------------

	/**
	 * Errori **strutturali** del catalogo. **Vuoto significa valido.** Non tocca il disco: gira ovunque,
	 * anche su una macchina senza i pack Paragon.
	 *
	 * Rifiuta, e ogni riga nomina il colpevole:
	 * catalogo assente · catalogo **vuoto** · `formatVersion` piu' nuovo della build · voce senza `id` ·
	 * `id` malformato · `id` duplicato · `assetPath` vuoto · `assetPath` duplicato ·
	 * `nextId` che non domina gli ID in uso.
	 *
	 * 🔴 **Un catalogo VUOTO non e' «zero mancanze»: e' la mancanza totale.** E' la stessa scelta di
	 * `URTIconLibrary::FindMissingRequiredIcons` e di `FindMissingBindings`, e serve perche' un gate che tace
	 * quando non trova nulla e' un gate che passa proprio nel caso peggiore — quello in cui nessuno ha ancora
	 * guardato una sola clip, che e' lo stato del progetto *prima* di questa issue.
	 *
	 * **Ordine deterministico**: le righe escono nell'ordine delle voci nell'array. Nessuna iterazione di
	 * `TMap`/`TSet` produce output — stessa disciplina di `FindMissingBindings`.
	 */
	static TArray<FString> ValidateCatalog(const FRTAnimCatalog* Catalog);

	/**
	 * I referenti che il disco **non** ha. Vuoto = tutti presenti, **ma solo se `bOutRan` e' vero**.
	 *
	 * 🔑 **`bOutRan` non ha un default e non e' opzionale, ed e' il punto di questa firma.** I pack Paragon
	 * sono gitignorati (~48 GB): su una macchina che non li ha, un `TArray` vuoto sarebbe **indistinguibile**
	 * da «tutti i referenti esistono». Con i pack assenti la funzione esce `bOutRan = false` e chi chiama
	 * **deve** scrivere `NOT RUN` — che non e' `PASS`, mai.
	 */
	static TArray<FString> ValidateReferents(const FRTAnimCatalog* Catalog, bool& bOutRan);
};
