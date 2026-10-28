#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Ability/RTActionDef.h"
#include "Map/RTCellId.h"
#include "Perception/RTTeamKnowledge.h"
#include "RTRangeVerdict.generated.h"

class URTHexMapAsset;

/**
 * Lo stato di una cella in portata, come lo vede la squadra che guarda (`#3750`, fetta B di F5; regola P7 della spec
 * dell'ispettore).
 */
enum class ERTRangeVerdict : uint8
{
	/** Tiro libero: la linea e' sgombra E la squadra vede adesso ogni cella della linea. */
	Free,

	/** Tiro bloccato: la linea e' interrotta E la squadra vede adesso ogni cella della linea. */
	Blocked,

	/** In portata, SENZA verdetto: la linea attraversa almeno una cella che la squadra non vede adesso. */
	Unverdicted,
};

struct REFACTORTACTICS_API FRTRangeCellVerdict
{
	FRTCellId Cell;
	ERTRangeVerdict Verdict = ERTRangeVerdict::Unverdicted;
};

/**
 * LA PORTATA DI UNA SKILL, COME VALORI (`#3750`, fetta D): da dove si mira, quanto lontano, quanto vicino, e se la linea e'
 * richiesta. E' tutto cio' che `URTRangeVerdictLibrary::ClassifyFor` chiede dell'azione, e nient'altro: confine con
 * `FRTFocusMoveSubject`, che fa lo stesso per l'area di movimento. Se due skill hanno gli stessi valori, per la mappa sono la
 * stessa portata, e passare dall'una all'altra non ricalcola niente.
 */
struct REFACTORTACTICS_API FRTRangeSubject
{
	/** La cella da cui si mira. */
	FRTCellId Origin;

	int32 RangeCells = 0;
	int32 MinRangeCells = 0;
	ERTLineOfSightPolicy Policy = ERTLineOfSightPolicy::Required;

	bool operator==(const FRTRangeSubject& Other) const
	{
		return Origin == Other.Origin
			&& RangeCells == Other.RangeCells
			&& MinRangeCells == Other.MinRangeCells
			&& Policy == Other.Policy;
	}
	bool operator!=(const FRTRangeSubject& Other) const { return !(*this == Other); }
};

/**
 * LA PORTATA DI UNA SKILL A TRE STATI: LIBERO, BLOCCATO, SENZA VERDETTO (`#3750`, fetta B; spec dell'ispettore §4 e P7).
 * La decisione D-552 copre l'AREA DI MOVIMENTO, non questa: da D-552 viene solo il principio «solo cio' che si vede adesso».
 *
 * Serve alla portata mostrata all'hover di una skill di un personaggio a fuoco. Per un nemico, dire «libero» o
 * «bloccato» di una linea che attraversa celle non in vista rivelerebbe geometria nascosta, o cio' che e' cambiato fuori
 * vista: per questo esiste il terzo stato.
 *
 * 🔴 **L'INSIEME delle celle in portata e' sotto lo stesso vincolo dei verdetti, ed e' la parte facile da dimenticare.**
 * La portata efficace dipende dal Fumo sulla linea (`URTTerrainLibrary::EffectiveTargetingRange`): un Fumo su una cella
 * non in vista restringerebbe l'insieme, e chi guarda lo leggerebbe dalla sua forma. Per questo insieme e verdetti si
 * calcolano su una mappa in cui ogni cella fuori da `VisibleCells` e' NEUTRA (pavimento, nessun blocco, nessun Fumo),
 * costruita come `URTHexSimLibrary::MakeOptimisticPlanningMap` ma dalla sola `VisibleCells`: mai `ExploredCells`, perche'
 * una cella esplorata e non piu' in vista porta il suo contenuto di adesso, che la squadra non puo' leggere (D-225, D-227).
 *
 * L'insieme coincide quindi con `URTCombatLibrary::TargetableRangeCells` su qualunque mappa in cui le celle non in vista
 * non hanno effetti (Fumo, blocchi): cella per cella, come la stessa azione armata. Dove le celle non in vista hanno un
 * effetto (un Fumo), l'insieme dell'hover e' PIU' GRANDE di quello armato, perche' la nube non lo restringe: per scelta la
 * privacy vince sulla fedelta' (scostamento dal §11 della spec, dichiarato nel piano della fetta).
 *
 * 🔑 **Chi protegge che cosa.** La neutralizzazione protegge l'INSIEME (il Fumo). I VERDETTI li protegge il controllo di linea:
 * su una linea interamente in vista la mappa neutra coincide con quella vera, quindi per i verdetti la neutralizzazione e'
 * difesa in profondita' e nessun test puo' osservarla da sola.
 *
 * ⚠️ **L'estensione della mappa non e' nascosta**: `ContainsCell` decide quali celle compaiono nell'insieme, e la mappa
 * neutra non toglie celle. Un buco in una zona mai vista si leggerebbe dalla forma; si considera pubblica l'estensione.
 *
 * Il verdetto si consulta SOLO per una linea interamente su celle in vista: la linea e' quella di `HexLine`, con la cella di
 * partenza e il bersaglio. Anche un tiro bloccato da un muro visibile resta senza verdetto se, oltre il muro, la linea
 * attraversa celle non in vista: e' la lettura piu' prudente della spec («il terzo stato copre ogni linea che attraversa celle
 * non in vista»), che per il caso «bloccato» chiede la sola cella che ferma il tiro.
 *
 * Il verdetto e' quello di MIRA, lo stesso di `URTCombatLibrary::ClassifyHexTargeting` e dell'azione armata: per una policy
 * che non chiede la linea (`NotRequired`, `BlindAimDirect`) ogni cella in portata e' libera, anche se in risoluzione la
 * traiettoria diretta si fermerebbe contro un muro. Quale policy passare lo decide il produttore del segnale (fetta D).
 *
 * ⚠️ **PRECONDIZIONE DEL CHIAMANTE: l'unita' che tira dev'essere OSSERVATA** (voce `Live`), non solo su una cella in vista.
 * Vale quanto detto per `URTFocusAreaLibrary`: la libreria riceve valori e non applica quel cancello.
 *
 * ⛔ Presentazione pura: nessuno stato canonico, niente hash, TurnLog o replay. Non cablata (fetta C).
 */
UCLASS()
class REFACTORTACTICS_API URTRangeVerdictLibrary : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * La mappa su cui si giudica: una copia in cui ogni cella fuori da `Knowledge.VisibleCells` e' neutra. Solo
	 * `VisibleCells`. `nullptr` se non c'e' mappa o se la squadra non vede nulla. Il chiamante tiene viva la copia e, in
	 * partita, la mette in cache.
	 *
	 * ⚠️ **La chiave della cache e' la mappa (e la sua revisione) PIU' `VisibleCells`.** Una copia costruita con un altro
	 * `VisibleCells` conserva il Fumo di una cella che ora non si vede, e l'insieme tornerebbe a dipendere da cio' che non
	 * si vede.
	 */
	static URTHexMapAsset* MakeVisibleOnlyNeutralMap(UObject* Outer, const URTHexMapAsset* Map, const FRTTeamKnowledge& Knowledge);

	/**
	 * Il calcolo su una mappa gia' neutralizzata. `NeutralMap` e `VisibleCells` devono venire dalla stessa conoscenza. Una
	 * cella di partenza che la squadra non vede non ha portata: lista vuota.
	 */
	static TArray<FRTRangeCellVerdict> Classify(const URTHexMapAsset* NeutralMap, const TArray<FRTCellId>& VisibleCells,
		const FRTCellId& From, int32 RangeCells, ERTLineOfSightPolicy Policy, int32 MinRangeCells = 0);

	/**
	 * Mappa neutra e calcolo insieme. Duplica la mappa a ogni chiamata: e' la scorciatoia dei test e di chi calcola una
	 * volta; per ingressi invariati si tiene `MakeVisibleOnlyNeutralMap` in cache e si chiama `Classify`.
	 */
	static TArray<FRTRangeCellVerdict> ClassifyFor(UObject* Outer, const URTHexMapAsset* Map, const FRTTeamKnowledge& Knowledge,
		const FRTCellId& From, int32 RangeCells, ERTLineOfSightPolicy Policy, int32 MinRangeCells = 0);
};
