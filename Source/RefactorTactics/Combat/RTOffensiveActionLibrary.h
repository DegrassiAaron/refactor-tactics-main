#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTOffensiveActionLibrary.generated.h"

class URTHexMapAsset;

/**
 * Perche' un attacco lineare si e' fermato dove si e' fermato.
 *
 * ⚠️ **Questa riga diceva «e' il reason code del TurnLog», e il TurnLog non lo riceve** — misurato il
 * 2026-09-01 (`#2035`): fuori da questa libreria e dai suoi test `ERTLineStop` **non ha consumatori**, e
 * `spec-turnlog.md` §12 registra che `BlockedByCover` fu *rimosso* dai reason del combat il 2026-08-03. E'
 * un'aspirazione scritta al presente, non un fatto — la stessa forma del dato dichiarato e mai letto che
 * `LineCells` documenta per `MaxTargetingRangeThrough`.
 *
 * Cio' che resta vero e' la DISCIPLINA: i valori si aggiungono **in coda**, perche' il giorno in cui il log
 * lo riceva davvero gli ordinali saranno gia' stabili, e il costo di farlo oggi e' zero.
 */
UENUM(BlueprintType)
enum class ERTLineStop : uint8
{
	/** Ha incontrato il primo bersaglio valido e lo ha colpito. */
	Hit,
	/** Ha percorso tutta la portata senza incontrare nessuno. */
	NoTarget,
	/**
	 * Interrotta da una CELLA che blocca la linea di tiro (`FRTHexCellData::bBlocksLineOfSight`): un ostacolo
	 * pieno, non una copertura.
	 *
	 * ⚠️ **Il nome dice «Cover» e il dato non e' una copertura, ed e' storia da non riscrivere.** Fino al
	 * 2026-09-01 questo valore era l'unico che potesse fermare la linea, e la sua riga diceva *«una COPERTURA
	 * ALTA: una cella che blocca la linea di tiro»* — due cose diverse messe insieme perche' il 2026-08-06,
	 * quando questo enum e' nato, `FRTHexCover` non esisteva ancora: nasce il giorno dopo con `CP 9.2`. Il
	 * valore nuovo e' `BlockedByEdgeCover`, in coda; questo conserva il significato che ha sempre avuto.
	 */
	BlockedByCover,
	/** Uscita dalla mappa: la linea finisce dove finisce il livello. */
	OffMap,
	/** Il punto mirato non sta su una delle sei direzioni esagonali: l'azione non parte. */
	NotAligned,
	/** Nessuna mappa autorevole: non si valuta e non si colpisce (fail-closed). */
	NoMap,

	/**
	 * Interrotta dalla GEOMETRIA INTRA-CELLA: un muro alto DENTRO una cella, non un ostacolo di cella
	 * (`D-269`, `D-270`, `#1830`).
	 *
	 * 🔑 **Vale un valore proprio perche' e' un difetto diverso da correggere per chi gioca**, ed e'
	 * esattamente cio' che il DoD di `#1830` chiede al combat log: *«non c'e' copertura»* e *«la geometria
	 * interna ha fermato il colpo»* portano a due gesti diversi — spostarsi di una cella nel primo caso,
	 * girare attorno al muro nel secondo. Confonderli rende il log una bugia, che e' la ragione gia' scritta
	 * per `NoTarget` / `OffMap` / `BlockedByCover`.
	 *
	 * ⚠️ **In CODA, e non e' estetica**: infilare un valore in mezzo rinumererebbe `OffMap`, `NotAligned` e
	 * `NoMap`. E' la disciplina che `ERTIntraCellTraversal` dichiara per se stesso — *«va aggiunto in coda
	 * insieme al suo produttore»* — e il produttore, qui, arriva nello stesso commit. Vale anche se oggi
	 * nessun log riceve questo enum: vedi il cappello sopra.
	 */
	BlockedByInteriorGeometry,

	/**
	 * Interrotta dal BORDO attraversato: `URTHexCoverLibrary::BlocksTraversal` — copertura ALTA o porta
	 * chiusa/bloccata (`CP 9.2`/`9.3`, `#2035`).
	 *
	 * 🔑 **E' il valore che il catalogo descriveva da sempre e che nessun ramo produceva.** `Line Attack`
	 * dice *«una copertura alta interrompe la linea»*, e la copertura sta sui BORDI: fino al 2026-09-01
	 * questa marcia leggeva solo `bBlocksLineOfSight`, che copertura non e'. Il gemello sulla vista e'
	 * `ERTLineOfSightBlock::EdgeBlocker`, e come quello **non distingue** il muro dalla porta: chi vuole il
	 * dettaglio chiede a `URTHexCoverLibrary`, che ne e' l'autorita'.
	 *
	 * ⚠️ **Vale un valore proprio perche' chiede un gesto diverso** da `BlockedByCover`: da un bordo ci si
	 * gira attorno o lo si apre, da un ostacolo di cella ci si sposta. E' il criterio gia' scritto per
	 * `BlockedByInteriorGeometry`.
	 */
	BlockedByEdgeCover
};

/** Esito di un attacco lineare: chi colpisce, dove passa e perche' si ferma. */
USTRUCT(BlueprintType)
struct FRTLineAttackResult
{
	GENERATED_BODY()

	/** Primo bersaglio valido incontrato (INDEX_NONE se nessuno). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	int32 HitUnitId = INDEX_NONE;

	/** Celle attraversate dal colpo, partenza esclusa, fino a dove si ferma (inclusa quella del bersaglio). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	TArray<FRTCellId> Cells;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	ERTLineStop Stop = ERTLineStop::NotAligned;

	FRTLineAttackResult() = default;
};

/**
 * Linea di soppressione PREPARATA da un'unita' (`Action.SuppressiveLine`): le celle controllate e il danno
 * che infliggono a chi ci entra.
 *
 * La zona e' un DATO prodotto nel Prep e consumato nel Move: fra le due fasi non cambia. Tenerla esplicita
 * (invece di ricalcolare la linea al momento del trigger) e' cio' che rende «una sola attivazione» una
 * proprieta' verificabile e non una questione di chi arriva prima.
 */
USTRUCT(BlueprintType)
struct FRTSuppressiveZone
{
	GENERATED_BODY()

	/** Chi ha preparato la linea. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	int32 OwnerUnitId = INDEX_NONE;

	/** Squadra del proprietario: la linea non scatta sui suoi. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	int32 OwnerTeamId = INDEX_NONE;

	/** Celle controllate (partenza esclusa), nell'ordine in cui la linea le percorre. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	TArray<FRTCellId> Cells;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	int32 Damage = 0;

	FRTSuppressiveZone() = default;
};

/** Il percorso a micro-step di un'unita' nella fase Move, come lo vede la soppressione. */
USTRUCT(BlueprintType)
struct FRTSuppressionMover
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Offense")
	int32 UnitId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Offense")
	int32 TeamId = INDEX_NONE;

	/** Celle attraversate in ordine, partenza esclusa: un elemento per micro-step. */
	UPROPERTY(BlueprintReadWrite, Category = "RefactorTactics|Offense")
	TArray<FRTCellId> Path;

	FRTSuppressionMover() = default;
};

/** Lo scatto di una linea di soppressione: chi lo prende, dove si ferma e a quale passo. */
USTRUCT(BlueprintType)
struct FRTSuppressionHit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	int32 VictimUnitId = INDEX_NONE;

	/** Cella APPENA RAGGIUNTA: chi fa scattare la linea ci resta, non torna indietro (catalogo §3). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	FRTCellId StopCell;

	/** Indice del micro-step che ha fatto scattare la linea (0 = il primo passo del turno). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	int32 StepIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Offense")
	int32 Damage = 0;

	/** Vero se la linea e' scattata davvero. */
	bool IsValid() const { return VictimUnitId != INDEX_NONE; }

	FRTSuppressionHit() = default;
};

/**
 * Le azioni offensive che hanno una GEOMETRIA propria (catalogo v0.1 §3): l'attacco lineare, che colpisce il
 * primo bersaglio valido, e la linea di soppressione, che colpisce chi ci cammina dentro.
 *
 * Non sono la `Shape::Line` delle abilita' d'archetipo, che colpisce TUTTI quelli attraversati: la differenza
 * e' esattamente cio' che il catalogo chiede («il primo bersaglio valido») e per questo sta in una funzione
 * propria invece che in un parametro in piu' di `CollectHexAttacks`.
 *
 * Funzioni pure: nessun Actor, nessun UWorld, nessuno stato. Fail-closed senza mappa autorevole.
 */
UCLASS()
class REFACTORTACTICS_API URTOffensiveActionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Celle percorse da una linea che parte da `From` verso `Toward`, partenza esclusa, al massimo
	 * `RangeCells`. Si ferma PRIMA di una cella che blocca la linea di tiro (la «copertura alta» del
	 * catalogo) e al bordo della mappa.
	 *
	 * `Toward` serve solo a scegliere una delle sei direzioni: la linea prosegue per tutta la portata anche
	 * se il punto mirato e' piu' vicino. Direzione non esagonale o mappa assente -> nessuna cella.
	 */
	static TArray<FRTCellId> LineCells(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& Toward,
		int32 RangeCells);

	/**
	 * `Action.LineAttack`: colpisce il PRIMO bersaglio valido sulla direzione mirata.
	 *
	 * `Occupancy` mappa cella -> UnitId; `Hostiles` sono gli UnitId ingaggiabili. Un'unita' che non e' in
	 * `Hostiles` (un alleato) non e' un bersaglio valido e **non ferma il colpo**: a interrompere la linea e'
	 * solo la copertura alta, che e' cio' che il catalogo dichiara.
	 */
	static FRTLineAttackResult ResolveLineAttack(const URTHexMapAsset* Map, const FRTCellId& From,
		const FRTCellId& Toward, int32 RangeCells, const TMap<FRTCellId, int32>& Occupancy,
		const TSet<int32>& Hostiles);

	/**
	 * `Action.SuppressiveLine` nella fase Prep: prepara la zona controllata lungo una direzione. Stessa
	 * geometria di `LineCells` — chi si copre da un attacco lineare si copre anche dalla soppressione.
	 */
	static FRTSuppressiveZone MakeSuppressiveZone(const URTHexMapAsset* Map, int32 OwnerUnitId, int32 OwnerTeamId,
		const FRTCellId& From, const FRTCellId& Toward, int32 RangeCells, int32 Damage);

	/**
	 * `Action.SuppressiveLine` nella fase Move: **una sola** attivazione per turno.
	 *
	 * Scatta sul primo nemico che entra in una cella controllata, dove "primo" e' un ordine TOTALE
	 * (micro-step piu' basso, poi UnitId piu' basso) e non l'ordine dell'array `Movers`: permutarlo non
	 * cambia chi la prende. La vittima si ferma nella cella appena raggiunta.
	 */
	static FRTSuppressionHit ResolveSuppression(const FRTSuppressiveZone& Zone,
		const TArray<FRTSuppressionMover>& Movers);
};
