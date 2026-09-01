#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTMatchStateHash.generated.h"

class URTHexMapAsset;
class ARTUnit;

/**
 * Stato di un'unità al termine della partita, nella forma che entra nel checksum.
 *
 * È una struttura di DATI e non `ARTUnit` perché il checksum non deve dipendere da un Actor: così si calcola
 * headless, si verifica con un test diretto, e chi lo legge vede esattamente cosa ci finisce dentro.
 */
USTRUCT()
struct FRTUnitStateDigest
{
	GENERATED_BODY()

	/**
	 * Identità dell'unità: **`ARTUnit::StableUnitId`**, lo stesso intero che il TurnLog porta in
	 * `FRTTurnLogEntry::UnitId` ([D-063](../../../docs/decisions/RT_PDR_00_Decision_Log.md)).
	 *
	 * ⚠️ **Era una `FString` presa dal file di scenario**, e cambiarla è stata una decisione
	 * ([D-084](../../../docs/decisions/RT_PDR_00_Decision_Log.md), issue `#490`): quell'identità **entra
	 * nell'hash**, e una partita vera non ha un file di scenario da cui prenderla. Con due sorgenti diverse
	 * lo stesso identico stato di fine partita dava **due hash diversi**, e il corpus golden non sarebbe mai
	 * stato confrontabile con una partita reale — che è precisamente ciò per cui il checksum finisce
	 * nell'archivio replay.
	 *
	 * Regge perché `StableUnitId` **non è l'ordine di spawn**: `ARTTurnManager::EnsureMatchRoster` lo deriva
	 * da `TeamId → cella iniziale → nome`, e l'ultimo criterio interviene solo fra due unità della stessa
	 * squadra sulla stessa cella, che una partita legale non produce. È funzione dello **stato iniziale**,
	 * quindi due mondi che allestiscono la stessa situazione assegnano gli stessi id.
	 *
	 * ⚠️ **Qui l'identità ENTRA nell'hash, mentre nel TurnLog resta fuori (D-063) — e non è una
	 * contraddizione**: è lo stesso criterio applicato a due oggetti diversi. Un campo entra nell'hash *se e
	 * solo se* due oggetti possono differire **solo** per quello. Due **tracce** non possono: l'unità che ha
	 * agito è già determinata dagli eventi, e `UnitId` serve a renderli spiegabili. Due **stati finali** sì:
	 * «l'unità 3 viva con 40 HP e la 7 caduta» e «la 7 viva con 40 HP e la 3 caduta» sono finali diversi, e
	 * l'unica cosa che li distingue è l'identità. Toglierla renderebbe indistinguibili due partite finite in
	 * modo opposto.
	 *
	 * ⚠️ Vale `0` finché `EnsureMatchRoster` non ha girato, cioè prima della **prima risoluzione**. Uno
	 * scenario che finisce senza risolvere nemmeno un turno ha tutti gli id a `0`: l'hash resta definito, ma
	 * perde il potere di distinguere le unità fra loro. Non è un caso da difendere qui — è un caso in cui
	 * non è ancora successo niente.
	 */
	UPROPERTY() int32 UnitId = 0;

	UPROPERTY() FRTCellId Cell;
	UPROPERTY() int32 Health = 0;
	UPROPERTY() int32 Shield = 0;
	UPROPERTY() int32 Energy = 0;
	UPROPERTY() bool bAlive = true;

	/**
	 * Stati attivi (tag). Arrivano da `TMap`/`TSet`, la cui iterazione non è deterministica: `HashMatchState`
	 * li **ordina** prima di mescolarli, quindi il chiamante può passarli come li trova.
	 */
	/**
	 * L'ORIENTAMENTO, che e' stato LOGICO e non presentazione — `D-261`, `#1800`.
	 *
	 * 🔴 **Senza, due stati che differiscono solo per dove guarda un'unita' davano lo stesso digest**,
	 * pur potendo produrre esiti futuri diversi: `Combat/`, `Perception/` e le reazioni il facing lo
	 * consumano. E' la stessa classe di difetto che `D-245` ha corretto per `ReactionResponse` — un hash
	 * che non discrimina cio' che il gioco legge.
	 *
	 * ⚠️ **E' il facing REALE, non `PlannedFacing`.** Quello e' un intento, vive nella pianificazione
	 * ed e' privato della squadra che lo dichiara: metterlo in un checksum di stato lo renderebbe
	 * confrontabile da chi non deve conoscerlo.
	 *
	 * ⛔ Non introduce un secondo sistema di facing: `D-243` resta l'owner delle sei direzioni di gioco, e
	 * i dodici spicchi restano presentazione e occupazione.
	 */
	UPROPERTY() ERTHexDirection Facing = ERTHexDirection::E;

	UPROPERTY() TArray<FName> Statuses;
};

/**
 * Checksum dello stato di fine partita (CP 12.1, issue #81).
 *
 * Copre unità, **stati**, **terreni modificati**, **strutture** e **progresso degli obiettivi**: prima
 * copriva le sole unità, e due finali che differivano solo per il campo — una cella in fiamme, una copertura
 * eretta, un obiettivo conquistato — davano lo stesso hash.
 *
 * ⚠️ **Cosa NON copre, e va saputo prima di fidarsene** (trovato in code review): copre il *valore corrente*
 * di terreni e strutture, non la loro **scadenza**. `ARTTurnManager` tiene separati `DynamicSurfaces`,
 * `DynamicArcs` e `DynamicCovers` — e' li' che vivono `TurnsRemaining` e, per le coperture, `FreeRotations`,
 * che decide se la prossima `Reconfigure` e' gratis. Due finali con la stessa copertura ma diversa durata
 * residua, o diverse rotazioni gratuite, danno oggi lo **stesso** hash: sono stati di gioco diversi che questo
 * digest non distingue. Non e' ricostruibile dalla mappa, perche' quei dati per scelta non ci stanno.
 *
 * Perché conta oltre al DoD: il corpus golden di CP 12.6 (#178) confronta TurnLog **e** checksum. Un checksum
 * cieco all'ambiente avrebbe delegato tutta la copertura ambientale al solo TurnLog, senza dirlo.
 */
UCLASS()
class REFACTORTACTICS_API URTMatchStateHashLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Digest FNV-1a dello stato finale: stesso idioma di `URTTurnLogLibrary::HashTurnLog`, e come quello
	 * mescola **solo interi** (invariante #4).
	 *
	 * Ordine totale e stabile: le unità per `Id`, le celle nell'ordine canonico dell'asset (`SortCells`), gli
	 * stati ordinati per nome. Permutare gli ingressi non cambia il risultato — è la proprietà che rende il
	 * checksum una prova di determinismo invece di una sua vittima.
	 *
	 * @param Map        mappa a fine partita: superfici, coperture, bordi, archi e revisione.
	 * @param Units      stato delle unità (l'ordine dell'array è irrilevante).
	 * @param TeamScores progresso obiettivo per squadra, indicizzato per `TeamId`.
	 */
	static uint32 HashMatchState(const URTHexMapAsset* Map, const TArray<FRTUnitStateDigest>& Units,
		const TArray<int32>& TeamScores);

	/**
	 * Costruisce i digest dalle unità. **Una sola implementazione, usata sia dall'harness sia dalla
	 * partita** ([D-084](../../../docs/decisions/RT_PDR_00_Decision_Log.md)).
	 *
	 * Duplicarla sarebbe il difetto che questa funzione esiste per evitare: due costruzioni che divergono
	 * anche di un campo producono hash diversi per lo stesso stato, e nessuno se ne accorge finché qualcuno
	 * non prova a confrontare un corpus con una partita.
	 *
	 * ⚠️ **Include le unità morte**, con `bAlive = false`. È il motivo per cui quel campo esiste: filtrarle
	 * renderebbe indistinguibili «tre vivi e un caduto» da «tre vivi e basta». Chi chiama deve quindi
	 * passare le unità **prima** che vengano distrutte — in `ARTTurnManager` significa prima di
	 * `DestroyDefeatedUnits`.
	 */
	static TArray<FRTUnitStateDigest> BuildUnitDigests(const TArray<ARTUnit*>& Units);
};
