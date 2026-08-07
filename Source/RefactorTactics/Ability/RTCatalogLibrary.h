#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"
#include "Turn/RTTurnRules.h"

class URTEquipmentData;
#include "RTCatalogLibrary.generated.h"

/**
 * Lettura e validazione del catalogo azioni: pura, deterministica, senza Actor e senza asset.
 *
 * Il validator e' una funzione pura per costruzione, cosi' a M11 puo' diventare un commandlet di CI senza
 * riscritture (stessa disciplina di URTHexMapAsset::ValidateMap).
 */
UCLASS()
class REFACTORTACTICS_API URTCatalogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Macro-fase di Atlas in cui l'azione risolve davvero. Funzione TOTALE: ogni valore dell'enum ha una
	 * macro-fase, nessun default silenzioso (il test lo verifica valore per valore).
	 *
	 * Rimappatura (ADR-0003 §3): Snapshot -> Planning · Preparation -> Prep · FastMovement -> **Dash** ·
	 * NormalMovement -> **Move** (dopo il Blast: qui il catalogo divergeva) · Control -> Blast ·
	 * Attack -> Blast · Environment -> Cleanup (dopo il Move) · Cleanup -> Cleanup.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static ERTMatchPhase MapResolutionPhase(ERTResolutionPhase Phase);

	/**
	 * Vero se l'azione e' una mobilita' RAPIDA, cioe' risolve nella macro-fase Dash (scatto, carica, salto,
	 * riposizionamento, corsa). E' l'unico gate di «questa azione e' uno scatto»: prima di #142 il resolver
	 * leggeva la fase e il resto del gioco un flag booleano sull'asset, e le due risposte divergevano — le
	 * azioni degli eroi dichiarano la fase e non il flag, quindi il bot non pianificava scatti per loro.
	 *
	 * Non dice COME ci si sposta: quello e' `FRTActionDef::MovementStyle`, ed e' cio' che distingue una
	 * mobilita' lineare da una a budget dentro la stessa fase.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static bool IsFastMovement(const FRTActionDef& Def);

	/** Codice numerico del catalogo (0/10/20/30/40/50/60): serve a rileggere i PDF, non alla risoluzione. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static int32 ResolutionPhaseCode(ERTResolutionPhase Phase);

	/**
	 * Errori strutturali di un catalogo di azioni (vuoto = catalogo valido). Rifiuta:
	 * ID assente o duplicato · priorita' negativa · portata/costo/cooldown negativi · azione che dichiara di
	 * risolvere nello Snapshot (fase di congelamento: nessuna azione risolve li') · azione di movimento con
	 * fallback diverso da `Stop` (regola del vertical slice: ci si ferma nell'ultima cella valida).
	 *
	 * Ogni messaggio nomina l'azione colpevole: un errore che non dice QUALE riga e' rotta costringe a
	 * ricontrollare tutto il catalogo a mano.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Catalog")
	static TArray<FString> ValidateActions(const TArray<FRTActionDef>& Actions);

	/**
	 * Errori strutturali di un insieme di equipaggiamenti (vuoto = valido). Rifiuta: id assente o duplicato ·
	 * cooldown negativo · **svantaggio non dichiarato**. Quest'ultima e' la regola di prodotto: un
	 * equipaggiamento senza svantaggio e' una scelta verticale, cioe' potere che si accumula.
	 */
	static TArray<FString> ValidateEquipment(const TArray<const URTEquipmentData*>& Equipment);

	/**
	 * Il catalogo di azioni realmente SPEDITO dal gioco: le definizioni che `ARTUnit` assegna agli archetipi.
	 * Serve al test che tiene allineati documento e codice — se un'azione perde l'ActionId o cambia fallback,
	 * il validator lo scopre in CI invece che in partita.
	 */
	static TArray<FRTActionDef> GetShippedActionCatalog();

	/** Definizione spedita con l'ActionId dato, o una definizione vuota se l'ID non e' nel catalogo. */
	static FRTActionDef FindShippedAction(const FName& ActionId);

	/**
	 * Le azioni GENERICHE del catalogo v0.1 (`Action.*`), quelle che non appartengono a un eroe. Lista
	 * separata da `GetShippedActionCatalog` perche' le due hanno regole diverse: quella spedita e' vincolata
	 * a cio' che `ARTUnit::ConfigureAsArchetype` assegna davvero (test di allineamento), questa e' il
	 * catalogo delle azioni che chiunque puo' dichiarare.
	 *
	 * Contiene le azioni fondamentali (§1), le mobilita' (§2) e le offensive (§3) del catalogo v0.1.
	 * Controllo, supporto e ambiente arrivano con CP 4.7 e le epic E8/E9.
	 */
	static TArray<FRTActionDef> GetCoreActionCatalog();

	/** Azione generica con l'ActionId dato, o una definizione vuota se l'ID non e' nel catalogo. */
	static FRTActionDef FindCoreAction(const FName& ActionId);

	/**
	 * Danno dell'attacco base per FASCIA di portata (catalogo v0.1 §1): corpo a corpo 28/r1 · corto 25/r3 ·
	 * medio 22/r4 · lungo 20/r6. Piu' lontano si colpisce, meno si fa male — e' la scelta orizzontale del
	 * catalogo, non una scala di potenza.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Catalog")
	static int32 BasicAttackDamageForRange(int32 WeaponRangeCells);

	/**
	 * `Action.BasicAttack` completata per una data arma: identita', fase, priorita' e fallback vengono dal
	 * catalogo, portata e danno dalla fascia. Un solo ActionId per tutti gli eroi, non quattro varianti.
	 */
	static FRTActionDef MakeBasicAttack(int32 WeaponRangeCells);

	/**
	 * Un'azione a bersaglio completata per una data arma: identita', danno, fase, priorita' e fallback
	 * vengono dal catalogo, la PORTATA dall'arma dell'eroe (il catalogo dichiara «bersaglio», non un numero).
	 * Portate degeneri ricadono su 1: un'azione a portata zero non colpirebbe nessuno.
	 */
	static FRTActionDef MakeWeaponAttack(const FName& ActionId, int32 WeaponRangeCells);

	/**
	 * `Action.PrecisionAttack` per una data arma: portata dell'arma **+1** (catalogo v0.1 §3), danno 24 dal
	 * catalogo. Il bonus e' l'identita' dell'azione, non un parametro del chiamante.
	 */
	static FRTActionDef MakePrecisionAttack(int32 WeaponRangeCells);

	/**
	 * Errori negli SLOT di un piano di turno di una singola unita' (vuoto = piano valido): due azioni non
	 * possono occupare lo stesso slot, e chi consuma entrambi (`Action.Sprint`) non lascia spazio a nessuna
	 * azione principale.
	 *
	 * E' qui che «PrecisionAttack non e' usabile dopo Sprint» diventa una regola generale invece di
	 * un'eccezione sull'ActionId: lo Sprint prende la principale, e l'attacco non la trova piu'.
	 */
	static TArray<FString> ValidateActionSlots(const TArray<FRTActionDef>& PlannedActions);
};
