#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTMatchSetupLibrary.generated.h"

class URTHexMapAsset;

/**
 * Una posizione di partenza dello scenario showcase: QUALE eroe, in QUALE squadra, su QUALE cella.
 * Legare i tre dati insieme evita l'ordine implicito di tre array paralleli, che si disallineano in silenzio.
 */
USTRUCT(BlueprintType)
struct FRTShowcaseSpawn
{
	GENERATED_BODY()

	/** `HeroId` del catalogo eroi (`Hero.Flux`, `Hero.Riva`, `Hero.Bastion`, `Hero.Vektor`), non un archetipo legacy. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Showcase")
	FName HeroId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Showcase")
	int32 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Showcase")
	FRTCellId Cell;

	FRTShowcaseSpawn() = default;
	FRTShowcaseSpawn(FName InHeroId, int32 InTeamId, const FRTCellId& InCell)
		: HeroId(InHeroId), TeamId(InTeamId), Cell(InCell) {}
};

/**
 * Funzioni PURE di allestimento della partita su mappa esagonale: scelta delle celle di partenza e
 * ricostruzione dell'occupazione dallo stato delle unita'. Nessuno stato, nessun Actor, nessun World:
 * il GameMode le chiama, i test le verificano headless.
 */
UCLASS()
class REFACTORTACTICS_API URTMatchSetupLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Celle di partenza per un NumPerTeam vs NumPerTeam sul layer indicato: le prime NumPerTeam al team 0,
	 * le successive al team 1. Prende le celle percorribili in ordine STABILE (Layer, X, Y) e assegna i due
	 * team dalle due estremita' dell'ordine, cosi' le squadre partono lontane senza dipendere da RNG.
	 * Mappa nulla o celle percorribili insufficienti -> array vuoto (il chiamante non allestisce a meta').
	 */
	static TArray<FRTCellId> PickStartCells(const URTHexMapAsset* Map, int32 NumPerTeam, int32 Layer);

	/**
	 * Arena di RIPIEGO: esagono pieno di raggio `Radius` sul layer 0, tutte le celle percorribili.
	 *
	 * Serve quando il livello non porta una mappa esagonale: senza, il GameMode spawnerebbe un
	 * `ARTHexMapActor` con l'asset vuoto e la partita non si allestirebbe — premere Play non mostrerebbe
	 * nulla. Non sostituisce una mappa d'autore: e' un fondo di scena giocabile, dichiarato nel log.
	 * `Radius < 1` o `Outer` nullo -> nullptr (nessuna arena a meta').
	 */
	static URTHexMapAsset* MakeDemoArena(UObject* Outer, int32 Radius);

	/**
	 * **Mappa di prova** generata da codice: l'arena che serve a esercitare le regole, non un fondo di scena.
	 * Esagono pieno di raggio 4 sul layer 0 con, in aggiunta:
	 * - celle che **bloccano il movimento** (ostacoli),
	 * - celle che **bloccano la vista** allineate fra le due meta' del campo (copertura/LOS),
	 * - una fascia di terreno **costoso** (Mud, costo 3) per vedere il budget mordere,
	 * - una **piattaforma sul layer 1** raggiungibile SOLO tramite un'unica transizione.
	 *
	 * Esiste in codice e non come `.uasset` per una ragione precisa: `Content/**` e' gitignorato, quindi una
	 * mappa dipinta a mano non sopravvive a un clone e non e' riproducibile fra macchine. Questa si'.
	 *
	 * `Outer` nullo -> nullptr.
	 */
	static URTHexMapAsset* MakeTestArena(UObject* Outer);

	/**
	 * **Arena della showcase «Il Relè» — versione Lite** (CP 15.2): la fixture d'integrazione riproducibile.
	 *
	 * Esagono pieno di raggio 5 sul layer 0 (91 celle), con le sole superfici gia' atterrate e nessuna
	 * regola nuova: acqua al centro, conduttivo a contatto, `Rough`, `Ice`, `Fire`, `Smoke`. Il layout e'
	 * **simmetrico rispetto al centro** — la superficie di `(q,r)` e quella di `(-q,-r)` coincidono — cosi'
	 * un esito non e' mai spiegabile con «quella meta' campo era piu' comoda».
	 *
	 * I costi di movimento li detta il catalogo terreni: la fixture non incide numeri propri.
	 *
	 * `Outer` nullo -> nullptr (nessuna arena a meta').
	 */
	static URTHexMapAsset* MakeShowcaseRelayLiteArena(UObject* Outer);

	/** Posizioni di partenza canoniche della showcase: Flux + Riva (team 0) contro Bastion + Vektor (team 1). */
	static TArray<FRTShowcaseSpawn> GetShowcaseRelayLiteSpawns();

	/**
	 * **Arena «Relay Basin» — la mappa CANONICA della showcase** (`RT_Showcase_Relay_v01`).
	 *
	 * 45 celle su un solo layer, forma irregolare (7 righe, `q` da -4 a +4 sulle due centrali), con l'obiettivo
	 * `Relay` a `(0,0,0)` e le due squadre agli estremi ovest ed est. Terreni: `Smoke` a ovest, lane
	 * `ShallowWater` -> `Conductive` a sud, `Rough` a sbarrare la via diretta est->Relay, `Fire` sulla soglia
	 * est, cresta `HighGround` a nord-est, ripiano `Ice` a sud. In piu': una **copertura bassa** sull'approccio
	 * nord al Relay e un **gate chiuso** (una porta, CP 9.3) sulla lane sud.
	 *
	 * **Non sostituisce `MakeShowcaseRelayLiteArena`.** Quella e' un esagono simmetrico che serve al
	 * determinismo; questa e' la mappa degli 8 turni. Rispondono a due domande diverse — «l'esito e'
	 * riproducibile?» e «il gioco sa mostrare cio' che dice di essere?» — e una non implica l'altra.
	 *
	 * Il layout e' **autorato** (`docs/product/showcase-v0.1.md` §2): la spec di scenario dichiarata
	 * dall'handoff non esiste nel repository. `RefactorTactics.ShowcaseRelay.BasinLayoutMatchesSpec` e' cio'
	 * che gli impedisce di derivare in silenzio.
	 *
	 * I costi di movimento li detta il **catalogo terreni**: la fixture non incide numeri propri.
	 *
	 * `Outer` nullo -> nullptr (nessuna arena a meta').
	 */
	static URTHexMapAsset* MakeShowcaseRelayBasinArena(UObject* Outer);

	/** Spawn canonici del Relay Basin: Flux `(-4,0)` + Riva `(-4,1)` contro Bastion `(4,0)` + Vektor `(4,1)`. */
	static TArray<FRTShowcaseSpawn> GetShowcaseRelayBasinSpawns();

	/**
	 * Occupazione cella -> UnitId ricostruita dallo stato delle unita': solo le vive occupano.
	 * I tre array sono paralleli; lunghezze incoerenti -> mappa vuota (input non fidato, nessun indovinare).
	 */
	static TMap<FRTCellId, int32> BuildOccupancy(const TArray<FRTCellId>& Cells,
		const TArray<int32>& UnitIds, const TArray<bool>& Alive);
};
