#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/RTIconCatalogData.h"
#include "RTIconLibrary.generated.h"

/**
 * Lettura e validazione del catalogo iconografico: pura, deterministica, senza Actor.
 *
 * Stessa disciplina di `URTCatalogLibrary` per le azioni — il validator e' una funzione pura per costruzione,
 * cosi' un catalogo rotto si scopre in CI e non a schermo.
 */
UCLASS()
class REFACTORTACTICS_API URTIconLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Prefisso obbligatorio di ogni `IconId`. Tiene separati lo spazio delle icone e quello dei Gameplay Tag. */
	static const TCHAR* IconIdPrefix;

	/**
	 * `UI.Icon.` + il percorso semantico che il gioco usa gia': `Action.Move` -> `UI.Icon.Action.Move`,
	 * `Status.Wet` -> `UI.Icon.Status.Wet`.
	 *
	 * Una sola regola per tutte le categorie: la chiave dell'icona si DERIVA dall'identificatore stabile che
	 * esiste, non si inventa accanto a lui. E' anche il motivo per cui un ID scritto a mano che sbaglia
	 * categoria e' un errore di validazione e non un mistero a runtime.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Icons")
	static FName MakeIconId(const FName& SemanticPath);

	/** Il nome della categoria come appare dentro l'`IconId` (`ERTIconCategory::Status` -> `Status`). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Icons")
	static FString CategoryName(ERTIconCategory Category);

	/**
	 * Le chiavi che il catalogo DEVE avere, derivate dai dati di gioco reali e non da una lista scritta a mano:
	 *
	 * - **Phase** — le quattro fasi volontarie del turno (`Prep`, `Dash`, `Blast`, `Move`). Planning e Cleanup
	 *   non sono fasi in cui il giocatore agisce, e Reaction non e' una fase.
	 * - **Action** — ogni azione di `URTCatalogLibrary::GetCoreActionCatalog()`: se un'azione e' pianificabile,
	 *   prima o poi l'HUD deve disegnarla.
	 * - **Status** — ogni tag registrato sotto `Status.` (`Core/RTGameplayTags.cpp`).
	 *
	 * Da qui viene il valore di `EveryKeyResolves`: l'insieme richiesto **non** e' il contenuto del catalogo,
	 * quindi il test non puo' passare per costruzione. Aggiungere un tag di stato o un'azione al catalogo fa
	 * cadere la copertura finche' l'icona non esiste — che e' il solo modo perche' questo dato resti vivo.
	 *
	 * Ordine deterministico: le fasi nell'ordine del turno, le azioni nell'ordine del catalogo, gli stati
	 * ordinati lessicalmente (l'ordine dei tag restituiti dal manager non e' garantito).
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Icons")
	static TArray<FName> RequiredIconIds();

	/**
	 * Le chiavi richieste che il catalogo non copre (vuoto = copertura completa). Un `Catalog` nullo le
	 * restituisce tutte: nessun catalogo non e' «zero mancanze».
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Icons")
	static TArray<FName> FindMissingRequiredIcons(const URTIconCatalogData* Catalog);

	/**
	 * Errori strutturali del catalogo (vuoto = catalogo valido). Rifiuta:
	 * `IconId` assente · senza il prefisso `UI.Icon.` · duplicato · categoria dichiarata diversa dal segmento
	 * dentro l'ID · asset nullo · missing-icon del catalogo non impostato.
	 *
	 * Ogni messaggio nomina la chiave colpevole: un errore che non dice QUALE riga e' rotta costringe a
	 * ricontrollare tutto il catalogo a mano.
	 *
	 * Non verifica la COPERTURA — quella e' `FindMissingRequiredIcons`. Sono due domande diverse: «questo
	 * catalogo e' coerente?» e «questo catalogo basta al gioco che abbiamo?».
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Icons")
	static TArray<FString> ValidateIconCatalog(const URTIconCatalogData* Catalog);

	/**
	 * Risolve una chiave in un asset. **Non restituisce mai un esito silenziosamente vuoto**: una chiave
	 * sconosciuta da' il missing-icon del catalogo con `bResolved = false` e una warning che nomina la chiave e
	 * il consumer.
	 *
	 * `Consumer` serve al log: senza, «icona mancante» in una partita con dieci widget non dice dove guardare.
	 *
	 * In Development la warning e' rumorosa apposta; in Shipping il comportamento e' lo stesso e non c'e' nulla
	 * che possa crashare — non si dereferenzia niente, si copia una soft reference.
	 */
	static FRTIconResolution ResolveIcon(const URTIconCatalogData* Catalog, const FName& IconId,
		const FName& Consumer);
};
