#pragma once

#include "CoreMinimal.h"
#include "Map/RTHexCellData.h"
#include "Turn/RTActionEvent.h"
#include "RTTerrainData.generated.h"

/**
 * Un'unita' raggiunta da una propagazione ambientale (CP 8.3), con la DISTANZA a cui e' stata raggiunta e il
 * danno che le spetta. E' il risultato di una funzione pura: descrive cosa succedera', non lo applica.
 *
 * `Steps` non e' un dettaglio diagnostico: e' la chiave d'ordine dichiarata dal catalogo terreni §2
 * (`distanza dalla sorgente -> CellId -> UnitId`) e distingue il colpo diretto (0) dalla propagazione (> 0),
 * che hanno danni diversi (20 / 12).
 */
USTRUCT(BlueprintType)
struct FRTPropagationHit
{
	GENERATED_BODY()

	/** Identita' stabile dell'unita' colpita (indice nello snapshot del turno). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 UnitId = INDEX_NONE;

	/** Passi sul grafo conduttivo dalla cella sorgente: 0 = sorgente, N = raggiunta dopo N celle. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 Steps = 0;

	/** Danno da applicare: iniziale a 0 passi, propagato oltre. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 Damage = 0;

	/** Cella in cui l'unita' e' stata raggiunta (per il TurnLog). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	FRTCellId Cell;

	FRTPropagationHit() = default;
};

/**
 * Definizione dichiarativa di un terreno del catalogo (RT_TerrainCatalog_v0.1.md §1): costo di movimento,
 * blocchi, conducibilita' elettrica, limite di targeting ed effetti applicati all'ingresso. Vive nel
 * catalogo letterale di URTTerrainLibrary::GetTerrainCatalog, non in un asset ne' in uno switch C++.
 */
USTRUCT(BlueprintType)
struct FRTTerrainDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	ERTHexSurface Surface = ERTHexSurface::Floor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 MoveCost = 1;

	/**
	 * Celle di scivolamento involontario per chi TERMINA il movimento qui (0 = nessuno; solo `Ice` lo usa).
	 * Sta nel catalogo e non in un `switch` perche' e' l'ultima regola di terreno che era ancora incisa
	 * nell'enum (DoD CP 8.1: "i costi vivono nei dati, non in `switch` C++").
	 *
	 * **Limite dichiarato (CP 8.1)**: `URTHexSimLibrary::ApplyIceSliding` legge questo campo come un booleano
	 * (`> 0` = scivola di UNA cella) e non srotola ancora N celle. Il valore intero e' la forma giusta del dato
	 * per le dinamiche di CP 8.4 (ignite/disgelo), non una funzionalita' consumata oggi.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 SlideCells = 0;

	/** Vieta Dash/Charge (mobilita' rapida) attraverso la cella; il movimento normale non e' affetto. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bBlocksDashCharge = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bBlocksLineOfSight = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bConductsElectricity = false;

	/**
	 * La superficie puo' PRENDERE fuoco (CP 8.4): `Action.Ignite` la trasforma in `Fire`, il fuoco vi si
	 * propaga. Il catalogo terreni §2 elenca come combustibili «vegetazione, olio, gas» — che **non esistono
	 * fra le otto superfici del v0.1** — ed esclude esplicitamente acqua e metallo.
	 *
	 * Conseguenza dichiarata: oggi il campo e' vero solo per `Floor` e `Rough`, cioe' il terreno neutro. Il
	 * fuoco non si propaga da solo perche' non c'e' combustibile che lo alimenti, non perche' manchi il codice:
	 * e' una proprieta' del CATALOGO, verificata da un test (`Environment.Fire.DoesNotIgniteWaterOrMetal`).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	bool bIsFlammable = false;

	/** 0 = nessun limite; N > 0 = la portata effettiva di un intento la cui linea attraversa questa cella e' min(RangeCells, N). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 MaxTargetingRangeThrough = 0;

	/** Effetti applicati a un'unita' quando entra nella cella (Damage/Status; riusa il vocabolario delle azioni). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	TArray<FRTActionEffectSpec> OnEnterEffects;

	/**
	 * Quanto questa superficie SPENDE del rumore che l'attraversa, oltre al passo (CP 13.3). `0` = terreno
	 * libero, che e' il riferimento della scala.
	 *
	 * Sta nel dato del terreno accanto a `MoveCost` — e non in una tabella dentro il propagatore — per la
	 * ragione che la DoD di CP 13.3 chiede per nome: *«nessuna intensita' incisa nel `TurnManager`»*. Una
	 * superficie nuova porta il proprio costo acustico con se', come porta il proprio costo di movimento;
	 * dimenticarselo la lascia a `0`, che e' il valore neutro e non una bugia.
	 *
	 * Derivato dal workbook (`08_Terreni` -> `Noise_Mod`) con `NoiseDelta = Noise_Mod - 1`, **con una
	 * eccezione decisa**: l'acqua bassa vale `+2` e non il `+3` che la formula darebbe (D-042 — vince il
	 * documento sorgente, il workbook resta `RESEARCH`). E' l'unico punto in cui formula e decisione divergono,
	 * ed e' pinnato da un test apposta.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Terrain")
	int32 NoiseDelta = 0;

	FRTTerrainDef() = default;
};
