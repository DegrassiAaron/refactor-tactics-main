#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTVisibilityBorder.generated.h"

/**
 * Un LATO ESPOSTO dell'unione visibile: la cella e la direzione attraverso cui, di la', non si vede.
 *
 * 🔑 **E' una coppia (cella, direzione) e non un segmento world-space**, ed e' la separazione che rende
 * questo modello testabile senza montare niente: la posa — punto e orientamento — si chiede a
 * `URTHexLibrary::EdgeMidpointWorld` / `EdgeRotation`, che li derivano dai due centri di cella. Una tabella
 * di sei angoli scritta qui si scollegherebbe da `AxialDirection` al primo cambio di convenzione degli
 * assi, e nessun compilatore lo direbbe.
 *
 * ⚠️ **`Cell` e' sempre la cella VISIBILE.** Il lato fra una cella vista e una non vista appartiene alla
 * prima: emetterlo da entrambe lo poserebbe due volte nello stesso punto, e a schermo sarebbe z-fighting.
 */
USTRUCT(BlueprintType)
struct FRTExposedEdge
{
	GENERATED_BODY()

	/** La cella VISIBILE che possiede il lato. Mai quella dall'altra parte. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	FRTCellId Cell = FRTCellId();

	/** Verso quale vicino il lato guarda: di la' la squadra non vede. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|Perception")
	ERTHexDirection Direction = ERTHexDirection::E;

	FRTExposedEdge() = default;
	FRTExposedEdge(const FRTCellId& InCell, ERTHexDirection InDirection)
		: Cell(InCell), Direction(InDirection) {}

	bool operator==(const FRTExposedEdge& Other) const
	{
		return Cell == Other.Cell && Direction == Other.Direction;
	}
};

FORCEINLINE uint32 GetTypeHash(const FRTExposedEdge& E)
{
	return HashCombine(GetTypeHash(E.Cell), GetTypeHash(static_cast<uint8>(E.Direction)));
}

/**
 * Il CONFINE dell'unione visibile di una squadra: da un insieme di celle ai suoi lati esposti.
 *
 * 🔑 **Pura e headless.** Nessun Actor, nessun `UWorld`, nessuna mappa: non consulta la LOS, non conosce il
 * terreno e non sa chi sia il viewer. Riceve un insieme di celle gia' deciso — `FRTTeamKnowledge::VisibleCells`
 * o `URTPerceptionLibrary::TeamVisibleCells` — e risponde dove finisce. Chiedere qui la LOS sarebbe la
 * seconda verita' sulla visibilita', ed e' esattamente cio' che questo file non fa.
 *
 * 🔴 **Nessuna semplificazione della forma.** Niente convex hull, niente cerchio dedotto da `VisionRange`:
 * il confine deve poter essere **concavo e disconnesso**, perche' lo e' — la fixture `VisionSplit` fa
 * nascere una squadra spezzata in due camere da un muro, e un contorno che le unisse affermerebbe il falso.
 *
 * ⚠️ **Il confine dell'ESPLORATO non e' questo.** [D-227] tiene i due stati distinti, e un secondo contorno
 * sullo stesso schermo li confonderebbe invece di separarli. Chi volesse il perimetro del ricordo passa
 * `ExploredCells` a questa stessa funzione **sapendo** di disegnare un'altra cosa.
 *
 * 🔑 **Perche' esiste, visto che #1715 ha misurato che il velo basta.** In partita basta: il salto di
 * luminosita' `1.0 / RTVeilExploredFactor` si legge a colpo d'occhio alla camera tattica. Il Tactical
 * Designer e' l'altro caso — camera libera, arene arbitrarie in authoring, e una prospettiva che si cambia
 * apposta per confrontare due squadre — ed e' li' che #1754 lo riprende. ⚠️ Il giudizio di #1715 resta
 * valido dove e' stato dato: chi tocca `RTVeilExploredFactor` tocca ancora quella decisione, non questa.
 */
UCLASS()
class REFACTORTACTICS_API URTVisibilityBorderLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * I lati esposti dell'insieme, in ordine STABILE: per cella (`URTHexLibrary::StableLess`) e, a parita' di
	 * cella, per direzione.
	 *
	 * Un lato fra due celle **entrambe** visibili e' interno e non compare mai. Un lato fra una cella
	 * visibile e qualunque altra cosa — cella non vista, oppure il vuoto oltre il bordo della mappa —
	 * compare **una volta sola**.
	 *
	 * 🔑 **Il vuoto conta come «non visibile», e non e' una scelta di comodo.** Questa funzione non ha la
	 * mappa, quindi non puo' distinguere «cella esistente ma non vista» da «fuori arena»: ed e' giusto cosi',
	 * perche' a schermo le due cose sono la stessa — di la' la squadra non vede. Una cella visibile isolata
	 * produce quindi **sei** lati, e l'insieme vuoto ne produce **zero**, mai «tutta la mappa».
	 *
	 * ⚠️ **Layer-aware per costruzione.** `URTHexLibrary::Neighbor` resta sul layer della cella, e due celle
	 * con la stessa `X/Y` su layer diversi non si annullano: ognuna porta i propri lati. Chi disegna puo'
	 * filtrare il layer inquadrato; questo modello no.
	 *
	 * L'esito e' invariante per permutazione dell'ingresso, come `TeamVisibleCells` che lo produce: i
	 * duplicati nell'ingresso non moltiplicano i lati.
	 */
	static TArray<FRTExposedEdge> ExposedEdges(const TArray<FRTCellId>& VisibleCells);
};
