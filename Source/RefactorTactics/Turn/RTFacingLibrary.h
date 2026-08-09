#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h" // ERTMovementStyle: le direzioni legali sono una proprieta' dello STILE
#include "Map/RTCellId.h"        // ERTHexDirection
#include "Turn/RTHexSim.h"       // FRTHexSimUnit: il facing autorevole vive nello snapshot
#include "Turn/RTTurnLog.h"      // ERTFacingOutcome: la timeline di D-020 e' la sequenza delle voci di log
#include "RTFacingLibrary.generated.h"

/**
 * Perche' un'unita' e' stata spostata. Le due cause producono orientamenti diversi e la differenza non e'
 * deducibile dalle celle: una spinta ha una sorgente verso cui girarsi, uno scivolamento no.
 */
UENUM(BlueprintType)
enum class ERTDisplacementCause : uint8
{
	/** Spinta, knockback, displacement da reazione: c'e' una sorgente. */
	Forced,
	/** Ghiaccio, corrente, terreno che cede: nessuna sorgente, nessuno verso cui voltarsi. */
	Environmental
};

/**
 * Regole PURE dell'orientamento (CP 16.1). Il facing e' stato di gioco autorevole, non lo yaw della mesh:
 * decide chi vede cosa (E13), da che lato si e' scoperti (CP 16.2) e dove punta un Overwatch (E14).
 *
 * Qui vivono solo le regole senza stato: quale direzione deriva da un percorso, quali rotazioni sono legali
 * per stile di movimento, e come reagisce l'orientamento a uno spostamento subito. La TIMELINE del facing
 * dentro il round (D-020) e la sua registrazione in snapshot/TurnLog non stanno in questa libreria: qui una
 * funzione riceve il facing corrente e restituisce quello nuovo, senza ricordare nulla fra una chiamata e
 * l'altra. E' cosi' che «un Move volontario vince sullo spostamento forzato» resta una proprieta'
 * dell'ORDINE in cui il resolver applica le regole, invece di diventare un flag nascosto qui dentro.
 *
 * Riferimento: ADR-0005 (orientamento), emendato da D-020 (piu' valori di facing per round).
 */
UCLASS()
class REFACTORTACTICS_API URTFacingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Direzione derivata da un percorso gia' risolto: quella dell'ULTIMO passo. Percorso vuoto o di una sola
	 * cella (l'unita' non si e' spostata) -> `Current` invariato.
	 *
	 * Se le ultime due celle non sono adiacenti — e' il caso del salto, che ignora le celle intermedie — vale
	 * la direzione VERSO l'arrivo. Non e' un caso speciale per stile: e' la stessa domanda («da dove a dove»)
	 * con una geometria piu' larga.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Facing")
	static ERTHexDirection FacingFromPath(const TArray<FRTCellId>& Path, ERTHexDirection Current);

	/**
	 * Le direzioni che l'unita' puo' assumere dopo essersi mossa con questo stile, in ordine STABILE
	 * (per valore dell'enum, non per ordine di scoperta):
	 *
	 * - `Linear*` -> UNA sola, quella del movimento: la mobilita' lineare non lascia scelta;
	 * - `Budget`  -> TRE: l'ultimo passo `D` e le due adiacenti `D±1` nel ciclo delle sei direzioni;
	 * - `None`    -> SEI: chi non si e' mosso ruota liberamente.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Facing")
	static TArray<ERTHexDirection> LegalFacings(ERTMovementStyle Style, const TArray<FRTCellId>& Path,
		ERTHexDirection Current);

	/**
	 * Applica una rotazione DICHIARATA in planning. `false` = illegale per lo stile: in quel caso `OutFacing`
	 * resta il facing corrente.
	 *
	 * Rifiutata, mai corretta in silenzio verso la legale piu' vicina: un piano che il giocatore non ha
	 * dichiarato e' un piano che non ha scelto, e in una fase simultanea non puo' nemmeno accorgersene.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Facing")
	static bool TryApplyDeclaredFacing(ERTMovementStyle Style, const TArray<FRTCellId>& Path,
		ERTHexDirection Current, ERTHexDirection Declared, ERTHexDirection& OutFacing);

	/**
	 * Orientamento dopo uno spostamento SUBITO. `Forced` -> ci si gira verso `SourceCell` (chi ha spinto);
	 * `Environmental` -> invariato. Sorgente sulla stessa cella d'arrivo -> invariato: non c'e' direzione.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Facing")
	static ERTHexDirection FacingAfterDisplacement(const FRTCellId& LandedCell, const FRTCellId& SourceCell,
		ERTDisplacementCause Cause, ERTHexDirection Current);

	/**
	 * Scrive il nuovo facing sull'unita' e ne registra la ragione nel TurnLog. Unico punto di scrittura: e'
	 * questo che rende la timeline di D-020 ricostruibile, perche' nessuno puo' cambiare l'orientamento senza
	 * lasciare la voce che dice quando e perche'.
	 *
	 * Scrivere lo STESSO valore che l'unita' ha gia' non produce nessuna voce: un non-cambiamento non e' un
	 * evento, e riempirne il log renderebbe l'hash del replay sensibile a scritture che non decidono niente.
	 * L'unica eccezione e' `DeclarationRejected`, che e' un esito osservabile proprio perche' NON cambia nulla.
	 */
	static void RecordFacingChange(FRTHexSimUnit& Unit, ERTHexDirection NewFacing, ERTFacingOutcome Reason,
		ERTMatchPhase Phase, TArray<FRTTurnLogEntry>& Log);

	/**
	 * Legge il facing per un consumatore (Blast, Overwatch) registrando CHI ha letto e QUALE valore. Restituisce
	 * sempre il valore autorevole piu' recente, cioe' quello sull'unita'.
	 *
	 * Esiste come funzione separata dalla lettura diretta del campo perche' la domanda a cui il replay deve
	 * saper rispondere non e' «che facing aveva» ma «che facing ha USATO il Blast»: senza traccia della lettura,
	 * un round con Dash e Blast nello stesso turno e' ambiguo.
	 */
	static ERTHexDirection ReadFacingForConsumer(const FRTHexSimUnit& Unit, ERTFacingOutcome Consumer,
		ERTMatchPhase Phase, TArray<FRTTurnLogEntry>& Log);
};
