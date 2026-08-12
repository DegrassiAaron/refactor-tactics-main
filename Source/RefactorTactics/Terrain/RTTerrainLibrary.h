#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Terrain/RTTerrainData.h"
#include "Combat/RTHexCombatLibrary.h" // FRTHexCombatUnit: le unita' dello snapshot, non un secondo tipo
#include "RTTerrainLibrary.generated.h"

class URTHexMapAsset;

/**
 * Lettura e validazione del catalogo terreni: pura, deterministica, senza Actor e senza asset.
 * Stesso schema di URTCatalogLibrary per le azioni (RT_TerrainCatalog_v0.1.md).
 */
UCLASS()
class REFACTORTACTICS_API URTTerrainLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Gli 8 terreni del catalogo v0.1 (RT_TerrainCatalog_v0.1.md §1). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static TArray<FRTTerrainDef> GetTerrainCatalog();

	/** Definizione del terreno indicato, o `FRTTerrainDef` di default (Floor) se assente dal catalogo. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static FRTTerrainDef FindTerrainDef(ERTHexSurface Surface);

	/**
	 * Portata di targeting EFFETTIVA fra due celle: il minimo fra `RangeCells` e il `MaxTargetingRangeThrough`
	 * dei terreni incontrati sulla linea di tiro (oggi solo il Fumo, cap 2).
	 *
	 * **Un solo posto** per questa regola: chi decide "il bersaglio e' a portata" DEVE passare di qui, altrimenti
	 * accetta un intento che il resolver poi scarta — l'azione spende lo slot e non succede nulla, senza una riga
	 * di log che lo spieghi.
	 *
	 * Estremi INCLUSI (cella dell'attaccante e del bersaglio): il DoD canonico dice «targeting max 2 celle
	 * **dentro o attraverso**», quindi stare NEL fumo cappa la propria portata quanto sparare attraverso.
	 * Diverge di proposito dalla convenzione di `URTHexVisionLibrary::HasLineOfSight`, che gli estremi li esclude.
	 *
	 * `Map == nullptr` -> `RangeCells` invariato: senza mappa non c'e' terreno da leggere, e il fail-closed lo
	 * decide il chiamante (che ha il proprio esito da registrare), non questa funzione.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Terrain")
	static int32 EffectiveTargetingRange(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
		int32 RangeCells);

	/**
	 * Stati che la superficie SOSTIENE finche' l'unita' ci sta sopra: gli `OnEnterEffects` di tipo Status che
	 * il catalogo dichiara con durata 0 («finche' sulla cella», CP 8.2). Oggi: `Wet` sull'acqua bassa,
	 * `Obscured` nel fumo.
	 *
	 * Applicazione (all'ingresso) e revoca (nel Cleanup) leggono questa STESSA funzione: se divergessero,
	 * uno stato potrebbe applicarsi e non essere mai revocato — o peggio il contrario, con l'unita' che si
	 * asciuga stando nell'acqua.
	 */
	static TSet<FGameplayTag> CellBoundStatusesFor(ERTHexSurface Surface);

	/** Errori strutturali del catalogo SPEDITO (vuoto = valido): id duplicato, costo o limite di targeting negativi. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Terrain")
	static TArray<FString> ValidateTerrainCatalog();

	/** Stessa validazione di ValidateTerrainCatalog, ma su un catalogo ARBITRARIO (testabile con input rotto). */
	static TArray<FString> ValidateCatalogEntries(const TArray<FRTTerrainDef>& Catalog);

	/**
	 * Chi viene raggiunto da una scarica elettrica partita da `SourceCell`, e con quanto danno (CP 8.3).
	 * PURA: nessun Actor, nessun UWorld, non applica nulla — calcola e restituisce, poi il chiamante applica.
	 *
	 * **Il percorso e' il grafo dell'acqua, non un raggio**: si cammina di cella in cella su superfici che
	 * dichiarano `bConductsElectricity` (acqua bassa, superficie conduttiva), al massimo `MaxSteps` passi
	 * (visita in ampiezza). La distanza esagonale darebbe un risultato diverso appena la pozza gira attorno a
	 * un ostacolo, e «l'elettricita' segue l'acqua» e' la regola che il giocatore puo' prevedere guardando la
	 * mappa — la leggibilita' tattica e' un pilastro di prodotto.
	 *
	 * Chi sta sulla `SourceCell` incassa `InitialDamage` (e' il bersaglio del colpo, non una propagazione);
	 * chi viene raggiunto oltre incassa `PropagatedDamage`. **Ogni unita' compare al massimo una volta**, anche
	 * quando piu' cammini d'acqua la raggiungono: e' il vincolo esplicito del catalogo terreni §2.
	 *
	 * La propagazione parte solo se la cella sorgente CONDUCE: elettrificare un bersaglio all'asciutto lo
	 * colpisce e basta. Le unita' non vive non vengono contate.
	 *
	 * Ordine del risultato — dichiarato dal catalogo e verificato da un test: **passi crescenti**, poi
	 * `URTHexLibrary::StableLess` sulla cella, poi `UnitId`. E' un ordine TOTALE: due esecuzioni con lo stesso
	 * input producono la stessa sequenza, quindi lo stesso TurnLog e lo stesso hash (invariante #4).
	 *
	 * `Map == nullptr` -> risultato vuoto (fail-closed, come la linea di tiro): senza mappa non si sa quali
	 * celle conducano, e inventarlo significherebbe danno arbitrario in partita.
	 */
	static TArray<FRTPropagationHit> CollectElectricPropagation(const URTHexMapAsset* Map,
		const FRTCellId& SourceCell, int32 MaxSteps, int32 InitialDamage, int32 PropagatedDamage,
		const TArray<FRTHexCombatUnit>& Units);

	/**
	 * Vero se **entrare** su questa superficie infligge danno (CP 7.5, `#505`).
	 *
	 * Criterio preso dal catalogo, non da un elenco scritto qui: la superficie e' pericolosa quando i suoi
	 * `OnEnterEffects` dichiarano un `Damage`. Oggi lo fa solo `Fire` (10 danni piu' `Burning`), e la
	 * definizione resta vera per la prossima superficie che nascera' dannosa senza che nessuno debba
	 * ricordarsi di aggiungerla.
	 *
	 * ⚠️ **Non basta `CellBoundStatusesFor`** per rispondere a questa domanda, ed e' l'errore naturale: quella
	 * restituisce solo gli stati SOSTENUTI (durata 0, «finche' sei sulla cella»), mentre `Burning` ha durata
	 * esplicita 2 e non compare. Una cella in fiamme risulterebbe innocua.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Terrain")
	static bool IsHazardousSurface(ERTHexSurface Surface);

	/**
	 * Dove fugge chi si trova su una cella diventata pericolosa (`Reaction.HazardEscape`, CP 7.5 `#505`), o
	 * `From` se non c'e' dove andare.
	 *
	 * **La prima scelta e' la cella verso cui l'unita' e' GIRATA.** Il facing lo dichiara il giocatore, quindi
	 * la fuga diventa prevedibile guardando il campo invece che arbitraria — e la leggibilita' tattica e' un
	 * pilastro di prodotto. Se quella cella non e' percorribile, sicura e libera, si ripiega sulle altre
	 * adiacenti nell'ordine canonico delle direzioni: deterministico, e dichiarato qui invece che emergente.
	 *
	 * Vincoli, tutti verificati sul grafo e non sulla distanza: la cella dev'essere **raggiungibile**
	 * (`GraphNeighbors`, quindi archi e dislivelli contano), non pericolosa (`IsHazardousSurface`) e non
	 * occupata. Senza mappa non si fugge — fail-closed, come ovunque qui.
	 */
	static FRTCellId FindEscapeCell(const URTHexMapAsset* Map, const FRTCellId& From,
		ERTHexDirection Facing, const TArray<FRTCellId>& Occupied);
};
