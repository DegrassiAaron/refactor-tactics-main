#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "RTStructureIdentityLibrary.generated.h"

class URTHexMapAsset;

/**
 * Un bordo risolto per nome: la cella che PORTA la voce di porta e la direzione occupata.
 *
 * E' la stessa convenzione con cui `FRTHexDoor` dichiara il proprio bordo — visto DALLA cella — perche' un
 * bordo scritto su una faccia sola resti citabile senza dover indovinare quale delle due l'ha ricevuto.
 */
USTRUCT(BlueprintType)
struct FRTStructureEdgeRef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Structures")
	FRTCellId Cell;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Structures")
	ERTHexDirection Edge = ERTHexDirection::E;

	FRTStructureEdgeRef() = default;
	FRTStructureEdgeRef(const FRTCellId& InCell, ERTHexDirection InEdge) : Cell(InCell), Edge(InEdge) {}

	bool operator==(const FRTStructureEdgeRef& Other) const
	{
		return Cell == Other.Cell && Edge == Other.Edge;
	}
};

/**
 * Un arco risolto per nome. Direzionale come `FRTHexEdge`: un ponte bidirezionale ne produce DUE, che
 * portano lo stesso nome perche' sono una struttura sola.
 */
USTRUCT(BlueprintType)
struct FRTStructureArcRef
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Structures")
	FRTCellId From;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Structures")
	FRTCellId To;

	FRTStructureArcRef() = default;
	FRTStructureArcRef(const FRTCellId& InFrom, const FRTCellId& InTo) : From(InFrom), To(InTo) {}

	bool operator==(const FRTStructureArcRef& Other) const
	{
		return From == Other.From && To == Other.To;
	}
};

/**
 * L'identita' stabile delle strutture di mappa (CP 23.3, #832): UNICO punto di lettura da nome a struttura.
 *
 * Sta accanto a `URTHexDoorLibrary` e non dentro di essa per la stessa ragione per cui quella sta accanto a
 * `URTHexCoverLibrary`: risponde a una domanda diversa — «quale struttura si chiama cosi'?» invece di
 * «questo varco e' aperto?» — e soprattutto perche' il nome copre anche gli ARCHI, che porte non sono.
 *
 * ⚠️ Nessuna seconda authority: chi deve MUTARE una porta continua a passare da
 * `URTHexDoorLibrary::SetDoorState`. Questa libreria risolve un nome, non cambia stato.
 */
UCLASS()
class REFACTORTACTICS_API URTStructureIdentityLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Bordi della porta che porta quel nome, in ordine canonico di cella.
	 *
	 * Vuoto se il nome non esiste, se e' `NAME_None` o se `Map` e' nullo: un nome che non risolve non e' un
	 * errore di lettura — e' il caso su cui poggia la validazione dei riferimenti pendenti.
	 */
	static TArray<FRTStructureEdgeRef> FindDoorEdges(const URTHexMapAsset* Map, FName StableId);

	/**
	 * IL NOME DELLA PORTA CHE STA SU QUEL BORDO — la risoluzione INVERSA di `FindDoorEdges`.
	 *
	 * 🔑 **Esiste perche' il runtime e il grafo parlano due vocabolari diversi**, e fino a `#833` non c'era
	 * nessun ponte fra loro. L'intento di gioco parla di BORDI — `FRTHexCombatIntent::DeclaredDoorEdge`,
	 * `FirstDoorEdge`, `FRTDoorOp{From, To}` — mentre il grafo di interazione e `ApplyInteraction` parlano di
	 * NOMI (`FName SourceId`). Chi punta una leva punta un bordo; chi la fa agire ha bisogno del suo nome.
	 *
	 * ⚠️ **Il bordo si guarda dai DUE lati**, come `CoverBetween` e per la stessa ragione: il dato sta su una
	 * cella sola, ma la struttura e' fisica e la regola non deve dipendere da quale delle due l'ha ricevuta.
	 *
	 * `NAME_None` se le celle non sono adiacenti, se non c'e' porta sul bordo, o se la porta non ha nome —
	 * e i tre casi non si distinguono, perche' per chi chiama sono la stessa cosa: **da qui non parte
	 * un'interazione remota**.
	 */
	static FName FindDoorIdOnEdge(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);

	/**
	 * QUEL NOME COMANDA QUALCOSA? Vero se compare come `SourceId` in un binding del grafo.
	 *
	 * ⛔ **Non dice se i bersagli risolvono**, e non e' una svista: quello lo sa `ResolveInteractionTargets`,
	 * che ha anche i reason code per dirlo. Questo predicato risponde alla sola domanda che il percorso di
	 * gioco deve fare **prima** di scegliere la strada — *«questa e' una leva o una porta?»* — e un binding
	 * rotto resta un difetto d'asset da riportare, non una leva che sparisce.
	 */
	static bool IsInteractionSource(const URTHexMapAsset* Map, FName StableId);

	/**
	 * Archi che portano quel nome, nell'ordine in cui stanno in `Transitions`.
	 *
	 * Vuoto alle stesse condizioni di `FindDoorEdges`. Un ponte bidirezionale ne restituisce due: sono i
	 * due versi della stessa struttura, non due strutture.
	 */
	static TArray<FRTStructureArcRef> FindArcs(const URTHexMapAsset* Map, FName StableId);

	/**
	 * Errori per i riferimenti che non risolvono: un bersaglio fantasma non deve arrivare a runtime.
	 *
	 * Un riferimento e' valido se il nome risolve una porta OPPURE un arco. `NAME_None` non e' un
	 * riferimento: e' un campo lasciato indietro, e va segnalato invece di risolversi in silenzio.
	 *
	 * ⚠️ In #832 non esiste ancora un dato di produzione che citi uno `StableId` — l'unico consumatore
	 * previsto e' l'interaction graph di CP 23.4 (#833). Questa funzione esiste perche' la regola viva in
	 * UN posto quando quel consumatore arrivera', invece di nascere dentro di lui.
	 */
	static TArray<FString> ValidateReferences(const URTHexMapAsset* Map, const TArray<FName>& References);

	/**
	 * Bersagli comandati da `SourceId`, **in ordine di applicazione** (CP 23.4, #833).
	 *
	 * L'ordine e' quello dichiarato in `FRTInteractionBinding::TargetIds`, e dentro ogni bersaglio quello che
	 * `FindDoorEdges` gia' garantisce. Nessuna `TMap` entra nel giro: l'invariante n. 3 non si difende con un
	 * `Sort` finale ma non costruendo mai un ordine da rimettere a posto.
	 *
	 * Sorgente sconosciuta o senza binding: array **vuoto**, che e' la risposta e non un errore — un
	 * `Interact` su una struttura che non comanda nulla e' legale e non fa niente.
	 *
	 * ⚠️ **Bersaglio conteso = risoluzione rifiutata**, con reason code in `OutErrors` se fornito. Se uno
	 * qualunque dei bersagli e' nominato anche da un'altra sorgente, l'esito e' vuoto: `INT-5` non ha deciso
	 * la semantica di composizione, e risolvere comunque la deciderebbe qui.
	 *
	 * 🔴 **Si rifiuta l'INTERA risoluzione, e la ragione NON e' piu' l'atomicita'.** Questa riga diceva
	 * *«perche' l'operazione su N bersagli e' dichiarata atomica»*, e [D-150] (2026-08-16) ha fatto cadere
	 * proprio quella premessa: si applicano i bersagli applicabili e si riporta l'esito degli altri. La
	 * ragione vera e' un'altra e sopravvive alla decisione: cio' che `INT-5` non ha deciso e' **chi comanda**
	 * quel bersaglio, non cosa fare dei bersagli rimasti. Un bersaglio conteso non ha un comandante
	 * riconosciuto, quindi non risolve — e insieme a lui non risolve il binding che lo nomina, perche'
	 * applicarne gli altri attribuirebbe a questa sorgente un'operazione parziale che nessuno le ha assegnato.
	 */
	static TArray<FRTStructureEdgeRef> ResolveInteractionTargets(const URTHexMapAsset* Map, FName SourceId,
		TArray<FString>* OutErrors = nullptr);

	/**
	 * Errori del grafo di interazione: bersaglio inesistente, binding duplicato per la stessa sorgente,
	 * sorgente vuota, binding senza bersagli, sorgente che comanda se stessa, e **bersaglio-arco**.
	 *
	 * ⚠️ **Due sorgenti che nominano lo stesso bersaglio NON sono un errore qui**: e' `INT-5`, aperta, e il
	 * dato deve poterla rappresentare. A rifiutarla e' la risoluzione con un reason code, non la validazione
	 * d'asset — altrimenti una decisione aperta riceve risposta da un validator.
	 *
	 * ⚠️ **Un bersaglio che risolve un ARCO fallisce**, ed e' il «binding cross-layer» che lo Scope di #833
	 * dichiara errore d'asset: gli archi sono l'unica struttura che collega layer diversi, e la risoluzione
	 * comanda solo porte. Senza la regola l'asset sarebbe valido e l'`Interact` inerte, in silenzio.
	 */
	static TArray<FString> ValidateInteractionGraph(const URTHexMapAsset* Map);
};
