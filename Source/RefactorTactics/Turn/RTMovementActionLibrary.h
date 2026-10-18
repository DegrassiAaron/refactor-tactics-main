#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Ability/RTActionDef.h"
#include "Map/RTCellId.h"
#include "RTMovementActionLibrary.generated.h"

class URTHexMapAsset;

/** Perche' una mobilita' lineare si e' fermata dove si e' fermata. E' il reason code del TurnLog. */
UENUM(BlueprintType)
enum class ERTLinearStop : uint8
{
	/** Arrivata alla cella richiesta. */
	Completed,
	/**
	 * Il bersaglio non ha una retta (la cella stessa, o un altro piano) o e' oltre la portata: l'azione non parte.
	 *
	 * ⌫ *Fino a [D-524] diceva «non e' su una delle sei direzioni».* Il nome e' rimasto — e con lui
	 * `ERTActionInvalidReason::DashNotAligned`, che viaggia come intero nel TurnLog — ma il significato e' cambiato:
	 * ogni cella dello stesso piano sta su una retta fra i centri, quindi l'allineamento non e' piu' un vincolo.
	 * Per una carica oltre la portata e' anche l'esito che il resolver restituisce quando le due condizioni di
	 * [D-516] (un nemico sulla cella, una retta libera) non reggono.
	 */
	NotAligned,
	/** Fermata davanti a un muro, a una cella che blocca il movimento o al bordo della mappa. */
	BlockedByTerrain,
	/** Fermata davanti a un'unita' (o perche' la cella d'arrivo era occupata). */
	BlockedByUnit,
	/**
	 * Fermata sulla cella ADIACENTE al primo nemico incontrato, che viene colpito: e' l'impatto della
	 * carica.
	 *
	 * ⚠️ **Non «sul» nemico**, che questa riga diceva fino a [D-296]: due unita' nella stessa cella non
	 * sono rappresentabili. Il ciclo di `ResolveLinearMove` esce **senza** far avanzare `Current`, quindi
	 * `Final` e' la cella precedente — e `HexMatch.ChargeStopsOnEnemyAndHits` lo pinna, asserendo
	 * `(2,0,0)` con il bersaglio su `(3,0)`.
	 */
	Impact,
	/**
	 * Fermata da un TERRENO che nega lo scatto pur restando percorribile a piedi: il dato e'
	 * `bBlocksDashCharge` (E8/CP 8.1), oggi l'accidentato.
	 *
	 * 🔑 **Ha un valore proprio e non riusa `BlockedByTerrain`**, che dice «muro, cella che blocca il
	 * movimento o bordo della mappa» — cioe' tre cose che si possono solo aggirare. Questa se ne distingue
	 * per la lezione che insegna: la via **esiste e si percorre**, ma non di corsa. Accorpandole, chi
	 * rilegge il turno va a cercare un muro che non c'e' — ed e' il difetto misurato il 2026-09-08 sulla
	 * seduta `U46`, dove la carica di `R_ROU` rifiutata dal rough di `(-2,-1)` arrivava al log come
	 * `resta`, indistinguibile da otto unita' che non avevano pianificato nulla.
	 *
	 * Valore aggiunto in CODA: le tracce gia' scritte conservano il proprio significato.
	 */
	TerrainDeniesDash,
	/**
	 * Una CARICA il cui bersaglio sta in linea ma **oltre la portata** ha percorso tutta la portata, senza
	 * ostacoli e senza impatti, e si e' fermata li' ([D-516]). Solo `LinearCharge`: per ogni altro stile un
	 * bersaglio oltre la portata resta `NotAligned`, e lo scatto non parte.
	 *
	 * 🔑 **Non e' un arrivo**: `Final` e' l'ultima cella della portata, non `Target`, e `IsLinearReachable`
	 * risponde falso. La carica non colpisce nessuno che non stia sulla linea entro la portata.
	 *
	 * Valore aggiunto in CODA, come `TerrainDeniesDash`.
	 */
	OutOfRange,
	/**
	 * Fermata da un BORDO fra due celle consecutive della retta: copertura alta o porta chiusa o bloccata
	 * (`URTHexCoverLibrary::BlocksTraversal`, [D-524] (5)).
	 *
	 * 🔑 **Ha un valore proprio e non riusa `BlockedByTerrain`**, per la stessa ragione di `TerrainDeniesDash`:
	 * la cella davanti e' libera, e chi rilegge il turno andrebbe a cercare un muro che non c'e'. Lo ferma
	 * `Dash`, `Charge` e `Pass`; il `Leap` guarda solo l'atterraggio e non lo incontra mai.
	 *
	 * Valore aggiunto in CODA, come `TerrainDeniesDash` e `OutOfRange`.
	 */
	BlockedByEdge
};

/** Esito di una mobilita' lineare: dove si arriva, cosa si attraversa, chi si colpisce e perche' ci si ferma. */
USTRUCT(BlueprintType)
struct FRTLinearMoveResult
{
	GENERATED_BODY()

	/** Cella finale (= cella di partenza se l'azione non parte). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	FRTCellId Final;

	/** Celle attraversate, partenza esclusa. Vuoto se non ci si muove. Per il salto contiene solo l'atterraggio. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	TArray<FRTCellId> Entered;

	/** Unita' urtata dalla carica (INDEX_NONE se nessuna). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	int32 ImpactUnitId = INDEX_NONE;

	/**
	 * Unita' ATTRAVERSATE lungo la traiettoria (`LinearPass`), nell'ordine in cui sono state incontrate.
	 * Vuoto per ogni altro stile.
	 *
	 * Lista e non singolo id, a differenza di `ImpactUnitId`: la carica si ferma AL primo bersaglio, quindi
	 * ne ha uno per definizione; la lama ne incontra quanti ne stanno sulla linea.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	TArray<int32> PassedThroughUnitIds;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	ERTLinearStop Stop = ERTLinearStop::NotAligned;

	/**
	 * [D-516] Vero se il bersaglio era una CARICA in linea ma oltre la portata, qualunque cosa l'abbia poi fermata.
	 *
	 * 🔑 **`Stop` da solo non lo dice**: un nemico sulla retta entro la portata da' `Impact` sia verso un bersaglio in
	 * portata sia durante la corsa verso uno lontano. Chi accetta un impatto come «la carica arriva» deve poterli
	 * distinguere senza rifare la geometria (review della PR #3767).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	bool bTargetBeyondRange = false;

	FRTLinearMoveResult() = default;
};

/**
 * [D-528] (`#3769`) Dove si ferma davvero una **carica pianificata**, e se finisce a contatto con il bersaglio che il giocatore ha
 * scelto. E' una lettura dell'esito di `ResolveLinearMove` (`DescribeChargeRun`), non una seconda geometria.
 *
 * 🔑 **La scrive chi pianifica e la leggono gli altri.** Chi pianifica (`ARTPlayerController::HandleClickOnCell`) ha la fotografia
 * della squadra ([D-371]) e quindi sa quali nemici la corsa incontrera'; l'anteprima e l'avviso del piano non hanno ne'
 * l'occupazione ne' la conoscenza, e ricalcolarla da lì userebbe la verita' — cioe' mostrerebbe un nemico che la squadra non conosce.
 *
 * `From`, `Target` e `Range` sono la CHIAVE: dicono per quale piano e' stata calcolata. Chi la legge scarta una lettura che non
 * corrisponde piu' al piano di adesso (`ARTUnit::GetPlannedChargeRun`) invece di disegnare una corsa vecchia.
 */
USTRUCT(BlueprintType)
struct FRTChargeRun
{
	GENERATED_BODY()

	/** La cella di partenza della carica per cui e' stata calcolata. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	FRTCellId From;

	/** La cella che il giocatore ha scelto (`PlannedDashCell`): quella del nemico cliccato, o una cella vuota. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	FRTCellId Target;

	/** La portata efficace con cui e' stata calcolata. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	int32 Range = 0;

	/** Le celle che la carica percorre davvero, la partenza esclusa. `0` se un nemico e' adiacente e la ferma subito. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	int32 RunCells = 0;

	/**
	 * Vero se la carica NON finisce a contatto col bersaglio scelto: si ferma alla portata, davanti a un ostacolo, o colpisce
	 * un'altra unita' ([D-528]). Il tratto di retta che resta fino al bersaglio e' quello che la carica non fara'.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	bool bMissesTarget = false;

	/** Vero se la corsa finisce con un impatto: `ImpactCell` dice su chi. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	bool bHasImpact = false;

	/** La cella dell'unita' colpita, che sta DOPO l'ultima cella percorsa ([D-296]). Valida solo con `bHasImpact`. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	FRTCellId ImpactCell;

	/**
	 * Vero se sulla cella scelta c'e' un nemico che la fotografia di chi pianifica conosce (`bEnemyOnTarget` del verdetto). La
	 * scrive `JudgeLinearDashClick`, che ha l'occupazione; `DescribeChargeRun` no.
	 *
	 * 🔴 **Chi nomina il bersaglio in un avviso lo fa SOLO se e' vero.** La cella scelta puo' essere vuota per la squadra e occupata per
	 * davvero da un nemico che non conosce: cercare un nome per cella fra tutte le unita' lo direbbe a chi prova la cella.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	bool bTargetIsKnownEnemy = false;

	FRTChargeRun() = default;
};

/**
 * [D-516] / [D-528] Il verdetto del click su una cella con una mobilita' lineare armata: la pianifica o la rifiuta, e perche'.
 *
 * 🔑 **E' la sede sola della domanda «questo click pianifica?»**, e la leggono il click (`ARTPlayerController::HandleClickOnCell`)
 * e l'hover (`BuildChargeHoverView`): [D-128] vuole che l'hover annunci cio' che il click esegue, e due letture della stessa
 * regola sono due regole. Il click sta sul bordo fra le due: scrive il piano; l'hover disegna cio' che il click scriverebbe.
 */
USTRUCT(BlueprintType)
struct FRTLinearDashVerdict
{
	GENERATED_BODY()

	/** Il click pianifica lo scatto verso la cella. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	bool bAccepted = false;

	/** L'esito del resolver sulla fotografia data: da qui il click ricava il motivo del rifiuto. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	FRTLinearMoveResult Linear;

	/** Sulla cella cliccata c'e' un nemico che la fotografia conosce. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	bool bEnemyOnTarget = false;

	/** Accettata perche' la carica urta un nemico entro la portata, verso un bersaglio che sta entro la portata. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	bool bImpact = false;

	/** Accettata perche' il bersaglio e' un nemico noto oltre la portata, su una retta libera ([D-516], [D-528]). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	bool bBeyondRange = false;

	/** Dove si ferma e se manca il bersaglio. Valorizzata per una `LinearCharge`, accettata o no. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Movement")
	FRTChargeRun Run;

	FRTLinearDashVerdict() = default;
};

/**
 * Le mobilita' LINEARI del catalogo v0.1 (`Dash`, `Charge`, `Leap`, `Reposition`): la RETTA libera fra i centri
 * di due celle ([D-523], [D-524]), non un percorso e non una delle sei direzioni — `MovementLine`.
 *
 * Sono cosa diversa dal movimento a budget (`Action.Move`, `Action.Sprint`), che aggira gli ostacoli in
 * pianificazione: qui la traiettoria e' dichiarata e cio' che la interrompe FERMA lo scatto. Tenerle separate
 * e' il motivo per cui `Dash.BlockedArc` e' verificabile: con il pathfinding, un muro davanti non fermerebbe
 * nulla — lo si girerebbe intorno.
 *
 * Funzioni pure: nessun Actor, nessun UWorld.
 */
UCLASS()
class REFACTORTACTICS_API URTMovementActionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Vero se lo stile dichiarato si muove in linea retta (e quindi passa da `ResolveLinearMove`). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Movement")
	static bool IsLinear(ERTMovementStyle Style)
	{
		return Style == ERTMovementStyle::LinearDash
			|| Style == ERTMovementStyle::LinearCharge
			|| Style == ERTMovementStyle::LinearLeap
			|| Style == ERTMovementStyle::LinearPass;
	}

	/**
	 * Vero se `To` sta su una delle sei direzioni esagonali a partire da `From` (stesso layer), con la
	 * distanza in celle. Il salto obliquo di un cavallo degli scacchi non e' una direzione esagonale.
	 *
	 * ⚠️ **Dal 2026-10-18 ([D-524] (1)) NON e' piu' la regola del movimento**: `Dash`, `Charge`, `Leap` e `Pass` leggono
	 * `MovementLine`, e ogni cella dello stesso piano sta su una retta. Questa resta degli ATTACCHI lineari
	 * (`LineCells`, `ResolveLineAttack`, la soppressione) — che seguono le sei direzioni — ed e' anche `BlueprintPure`,
	 * quindi non si cambia sul posto: i chiamanti Blueprint non sono contabili da qui.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Movement")
	static bool IsStraightLine(const FRTCellId& From, const FRTCellId& To, int32& OutDistance);

	/**
	 * **La retta del movimento lineare** ([D-523], [D-524] (2)): le celle di `URTHexLibrary::HexLine` da `From` a `To`,
	 * ESTREMI INCLUSI, col suo spareggio intero. `OutLine.Num() - 1` e' la distanza esagonale, cioe' la portata che
	 * `Dash`, `Charge`, `Leap` e `Pass` misurano.
	 *
	 * 🔑 **E' l'UNICA funzione che decide la retta del movimento**, e la leggono tutti: il giocatore (il click e il verso
	 * pianificato), il resolver (`ResolveLinearMove`), l'anteprima (HUD e fantasma della timeline) e i bot. Una seconda
	 * copia nel bot o nell'HUD sarebbe una seconda autorita'; l'hanno gia' pagata altri sottosistemi.
	 *
	 * Falso — e `OutLine` vuota — se `From == To` o se i due stanno su piani diversi: `HexLine` e' planare e ignora il
	 * layer d'arrivo, quindi il vincolo di piano di `IsStraightLine` si tiene qui.
	 *
	 * ⚠️ **Non sostituisce `IsStraightLine`**, che resta delle sei direzioni: la leggono `URTOffensiveActionLibrary::LineCells`,
	 * `ResolveLineAttack` e la soppressione, e gli attacchi lineari non seguono questa retta ([D-524] (1)).
	 */
	static bool MovementLine(const FRTCellId& From, const FRTCellId& To, TArray<FRTCellId>& OutLine);

	/**
	 * Risolve una mobilita' lineare secondo lo stile dichiarato dall'azione.
	 *
	 * La traiettoria e' `MovementLine`, e la portata `MaxCells` e' la distanza esagonale ([D-524]).
	 *
	 * - `LinearDash` — avanza finche' la cella successiva e' percorribile e libera; si ferma PRIMA di un muro,
	 *   di un'unita' o di un BORDO che nega l'attraversamento (copertura alta, porta chiusa o bloccata:
	 *   `BlockedByEdge`). Non termina mai in una cella occupata.
	 * - `LinearCharge` — come sopra, ma se la cella successiva ospita un NEMICO si ferma li' davanti e lo
	 *   segnala come impatto: e' la carica che si arresta addosso al bersaglio. E un bersaglio sulla retta ma
	 *   **oltre** `MaxCells` non la annulla: corre per tutta la portata e si ferma con `OutOfRange` ([D-516]).
	 * - `LinearLeap` — ignora unita', celle intermedie e bordi: conta solo l'atterraggio, che deve essere una
	 *   cella percorribile e libera. Non cambia layer.
	 * - `LinearPass` — come lo scatto, ma attraversa le unita' (tranne sulla cella d'arrivo).
	 *
	 * `Occupancy` mappa cella -> UnitId; `Hostiles` sono gli UnitId che la carica considera bersagli.
	 * Senza mappa non si muove nessuno (FAIL-CLOSED): non si sa cosa ci sia davanti.
	 */
	static FRTLinearMoveResult ResolveLinearMove(const URTHexMapAsset* Map, const FRTCellId& From,
		const FRTCellId& Target, int32 MaxCells, ERTMovementStyle Style,
		const TMap<FRTCellId, int32>& Occupancy, const TSet<int32>& Hostiles);

	/**
	 * Vero se una mobilita' lineare con questi parametri **arriva davvero** su `Target`.
	 *
	 * Serve a chi PROPONE uno scatto (il bot genera le candidate col grafo, che non conosce la linearita')
	 * per non offrire mosse che la risoluzione rifiuterebbe: un'abilita' spesa senza effetto e senza
	 * spiegazione e' peggio di una mossa non fatta. E' un semplice predicato su `ResolveLinearMove`, non una
	 * seconda implementazione — e' proprio la divergenza fra le due che la issue #140 ha chiuso.
	 *
	 * `From == Target` e' vero: «resto dove sono» non e' uno scatto illegale, e chi filtra un insieme di
	 * candidate non deve perdere quella di partenza.
	 *
	 * Nota sulla CARICA: qui conta l'arrivo sulla cella, quindi una carica che si ferma ADDOSSO al nemico
	 * (esito `Impact`, cella precedente) risulta non raggiungibile. E' voluto: chi filtra candidate di
	 * riposizionamento vuole sapere dove si ATTERRA, e l'impatto e' un intento del Blast, non un arrivo.
	 */
	static bool IsLinearReachable(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& Target,
		int32 MaxCells, ERTMovementStyle Style, const TMap<FRTCellId, int32>& Occupancy,
		const TSet<int32>& Hostiles);

	/**
	 * Vero se fra `From` e `Target` c'e' una **retta libera dal terreno e dai lati**, a qualunque distanza: ogni
	 * cella, `Target` compreso, si attraversa di corsa, e nessun bordo la nega ([D-516], [D-524] (5)).
	 * Falso se `MovementLine` non ha una retta (la cella stessa, un altro piano).
	 *
	 * 🔑 **Le unita' non contano, e la portata nemmeno.** E' la domanda che la pianificazione fa per una carica
	 * su un nemico OLTRE la portata: *«c'e' un percorso lineare fino a lui?»*. Chi sta sulla linea si sara' mosso
	 * quando la carica risolve; un muro no.
	 *
	 * ⚠️ **Non e' una seconda geometria**: e' `ResolveLinearMove` con un passo per ogni cella della retta e
	 * un'occupazione vuota, quindi muri, bordo e terreno che nega la corsa sono gli stessi della risoluzione.
	 */
	static bool IsLineOpenToTarget(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& Target);

	/**
	 * [D-528] Legge dall'esito di `ResolveLinearMove` dove si ferma una carica e se arriva a contatto col bersaglio scelto.
	 *
	 * 🔑 **Riporta, non decide**: il numero di celle e l'unita' urtata sono quelli del resolver, sulla fotografia che gli e'
	 * stata data. Chi la chiama con la fotografia di chi pianifica ottiene cio' che la squadra puo' sapere; con quella
	 * onnisciente, un'informazione che non le spetta ([D-225]).
	 */
	static FRTChargeRun DescribeChargeRun(const FRTLinearMoveResult& Linear, const FRTCellId& From, const FRTCellId& Target,
		int32 Range);

	/**
	 * [D-516] / [D-528] Il click su `Cell` con questa mobilita' lineare armata: la pianifica? Sulla fotografia di chi pianifica
	 * (`Occupancy`, `Hostiles`), cioe' su cio' che la squadra conosce ([D-371]).
	 *
	 * Si pianifica se la retta arriva sulla cella; oppure, per una carica, se urta un nemico entro la portata verso un bersaglio che
	 * sta entro la portata; oppure se il bersaglio e' un NEMICO NOTO oltre la portata e la retta fino a lui e' libera dal terreno
	 * e dai lati — che la corsa arrivi in fondo alla portata o la fermi prima un altro nemico noto ([D-528]). Una cella vuota
	 * oltre la portata resta rifiutata: la regola e' «caricare un nemico», non «correre verso un punto».
	 */
	static FRTLinearDashVerdict JudgeLinearDashClick(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& Cell,
		int32 DashRange, ERTMovementStyle Style, const TMap<FRTCellId, int32>& Occupancy, const TSet<int32>& Hostiles);
};
