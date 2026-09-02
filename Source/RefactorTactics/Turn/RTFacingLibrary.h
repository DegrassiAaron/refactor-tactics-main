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
 * Da quale dei sei lati un colpo arriva, RELATIVAMENTE all'orientamento di chi lo subisce ([D-126], `FAC-11`).
 * Le quattro direzioni non frontali restano DISTINTE: niente `Side`/`Flank` generico, e nessuna banda globale
 * `Front Arc / Flank / Rear`. Un'abilita' puo' raggruppare i lati che le servono, ma quell'insieme appartiene
 * al CONSUMATORE che lo dichiara, mai al canone.
 *
 * 🔑 **L'ORDINE DI QUESTO ENUM E' LA MAPPATURA nome<->indice, ed e' una scelta, non una trascrizione.**
 * [D-147] la lascia esplicitamente aperta — *«si fissa quando la relazione entra in codice, ed e' lavoro di
 * #726»* — perche' le due fonti si contraddicono e il verso NON e' deducibile dai nomi. Misurato:
 * `AxialToWorld` da' `Wx = sqrt(3)*(q + r/2)`, `Wy = 1.5*r`, quindi con facing `E` la direzione `NE`
 * (l'indice `f+1`) cade in world a `(+0.87, -1.5)` — cioe' a **-Y**, che nella convenzione UE (`+X` avanti,
 * `+Y` a destra) sta a **SINISTRA** di chi guarda. L'elenco di [D-126] letto come ordine di enumerazione
 * manderebbe `f+1` a `FrontRight`, cioe' nominerebbe destra una cella che sta a sinistra.
 *
 * ∴ Si sceglie la FEDELTA' GEOMETRICA: i sei nomi di [D-126] percorsi in senso INVERSO. [D-126] fissa i
 * nomi, non il verso di enumerazione; e l'argomento residuo che [D-147] registra sullo skew e' proprio la
 * *fedelta' dei nomi* — un `TurnLog` che dice `FrontRight` per una cella visibilmente a sinistra e' il
 * difetto di explainability (E16) che quell'argomento teme.
 *
 * ⚠️ **Il valore intero e' `(spicchio - facing + 6) % 6`**, e i test asseriscono su QUELL'INDICE, non su
 * questi nomi: la mappatura e' l'oggetto della decisione, quindi non puo' essere anche la sua premessa.
 *
 * ⚠️ Lo SKEW e' reale e dichiarato, non nascosto ([D-147]): `Front` e' il raggio dritto davanti piu' uno
 * solo dei due spicchi adiacenti, e a raggio `1..8` **168 celle su 216** non ricevono la direzione speculare
 * della propria immagine speculare. Uno `Shield = {Front}` proteggerebbe un fianco e non l'altro. Chi
 * dichiarera' il primo insieme di lati deve saperlo PRIMA di sceglierlo, non scoprirlo dal playtest.
 */
UENUM(BlueprintType)
enum class ERTRelativeDirection : uint8
{
	/** `(spicchio - facing) % 6 == 0` — il raggio dritto davanti, piu' lo spicchio alla sua sinistra. */
	Front,
	/** `== 1` — verso `-Y` in world, cioe' a sinistra di chi guarda. */
	FrontLeft,
	/** `== 2` */
	RearLeft,
	/** `== 3` — l'opposto esatto di `Front`. */
	Rear,
	/** `== 4` */
	RearRight,
	/** `== 5` — verso `+Y` in world, cioe' a destra di chi guarda. */
	FrontRight
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
	 * Da quale dei sei lati, relativamente a `Facing`, il difensore in `DefenderCell` e' raggiunto da qualcosa
	 * che parte da `OriginCell` ([D-126], regola a settore semiaperto confermata da [D-147]).
	 *
	 * `false` quando le due celle coincidono IN PIANTA: li' non esiste un lato d'ingresso. Non e' un caso
	 * d'errore — `URTHexCombatLibrary::IsInFrontalArc` alla stessa domanda risponde `true` per contratto
	 * («nessun lato da cui il colpo arrivi», quindi non e' alle spalle) — ma le due risposte non sono in
	 * conflitto: quella dice *se* sei coperto, questa *da dove*, e a distanza zero il «da dove» non c'e'.
	 *
	 * ⚠️ **E non e' un caso raro**: `AimCell` di un'area centrata su un'unita' E' la cella di quell'unita',
	 * quindi il bersaglio al centro di un'esplosione ricade sempre qui. Chi conta «una direzione per colpo»
	 * trova un buco, e il buco e' la risposta giusta.
	 *
	 * ⚠️ **Su `false`, `OutDirection` resta INVARIATA** — stessa convenzione di `URTHexLibrary::DirectionBetween`.
	 * Il chiamante guarda il `bool`; da Blueprint un grafo che ignora il pin legge lo zero-inizializzato,
	 * cioe' `Front`.
	 *
	 * PLANARE, come `IsInFrontalArc`: l'origine si proietta sul layer del difensore. Ricalcolare
	 * diversamente farebbe divergere le due funzioni su un caso che nessun test copre.
	 *
	 * ⛔ **Non sostituisce `IsInFrontalArc` e non ne cambia nessun chiamante.** Il cono a 120 gradi e'
	 * STRETTAMENTE CONTENUTO nell'insieme dei tre lati frontali — 45 celle di divergenza a raggio `1..10`,
	 * tutte nel verso «tre lati dentro / cono fuori», zero nel verso opposto — quindi spostare la copertura o
	 * la `Guard` su questa relazione sarebbe un BUFF DIFENSIVO NETTO travestito da rinomina. [D-126] tiene le
	 * due cose separate apposta: il cono dice *quale area* un'unita' copre, questa *da quale lato* e' colpita.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Facing")
	static bool RelativeDirectionFrom(const FRTCellId& DefenderCell, ERTHexDirection Facing,
		const FRTCellId& OriginCell, ERTRelativeDirection& OutDirection);

	/**
	 * Scrive il nuovo facing sull'unita' e ne registra la ragione nel TurnLog. Unico punto di scrittura: e'
	 * questo che rende la timeline di D-020 ricostruibile, perche' nessuno puo' cambiare l'orientamento senza
	 * lasciare la voce che dice quando e perche'.
	 *
	 * Scrivere lo STESSO valore che l'unita' ha gia' non produce nessuna voce: un non-cambiamento non e' un
	 * evento, e riempirne il log renderebbe l'hash del replay sensibile a scritture che non decidono niente.
	 * L'unica eccezione e' `DeclarationRejected`, che e' un esito osservabile proprio perche' NON cambia nulla.
	 */
	/**
	 * ⚠️ **Le voci prodotte NON hanno contesto**: `Phase`, `Category`, `Outcome`, `SrcCell`, `TgtCell` e
	 * `Amount` si riempiono qui; `TurnNumber`, `GraphRevision` e `UnitId` no, e da `FRTHexSimUnit` non si
	 * possono dedurre — porta l'indice della simulazione, non `StableUnitId`.
	 *
	 * Passare `ARTTurnManager::TurnLog` direttamente a questo `Log` e' il difetto di `#1429`: le voci
	 * entravano nella traccia con turno 0, revisione del grafo 0 e nessuna unita'. Dal manager si chiama
	 * `ARTTurnManager::RecordFacingChange`, che travasa con `AppendLogEntry`.
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
