#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
#include "RTHexArcLibrary.generated.h"

class URTHexMapAsset;

/** Cosa e' cambiato su un arco: le voci che il chiamante scrive nel TurnLog. */
USTRUCT(BlueprintType)
struct FRTArcChange
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId From;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	FRTCellId To;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	ERTHexArcState State = ERTHexArcState::Active;

	/** Integrita' RESIDUA dopo il cambio (per il log: quanto manca al crollo). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	int32 RemainingIntegrity = 0;

	/** Vero se dopo il cambio il collegamento non si percorre piu'. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Hex")
	bool bBroken = false;
};

/**
 * Gli archi come oggetti di gioco (CP 9.4): stato, integrita', conduttivita'.
 *
 * Gemella di `URTHexDoorLibrary` e con la stessa disciplina — un solo punto di lettura, un solo punto di
 * mutazione — ma risponde a una domanda opposta: l'arco e' ADDITIVO (crea un collegamento che non esiste),
 * la porta e' sottrattiva (nega un'adiacenza che esiste). E' la ragione per cui sono due cose diverse, e la
 * decisione e' registrata in docs/gameplay/spec-porte-cp93.md §1.
 *
 * Un arco non attivo NON ha un'adiacenza planare di riserva: le due celle tornano irraggiungibili e il
 * percorso fallisce, mentre una porta chiusa si aggira.
 */
UCLASS()
class REFACTORTACTICS_API URTHexArcLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Arco From->To, o nullptr se non esiste. */
	static const FRTHexEdge* FindArc(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);

	/**
	 * Vero se fra le due celle c'e' un collegamento PERCORRIBILE. Considera l'arco nei due versi: `ModifyArc`
	 * crea coppie bidirezionali, e chi chiede «si passa?» non deve sapere quale dei due versi e' stato scritto.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool IsArcTraversable(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);

	/** Vero se l'elettricita' risale il collegamento: arco ATTIVO che dichiara la conduttivita'. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool ArcConductsElectricity(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To);

	/**
	 * UNICO ingresso di mutazione dello stato. Porta l'arco — e il suo gemello inverso, se c'e' — allo stato
	 * richiesto, con UNA sola revisione.
	 *
	 * Regole di transizione, qui perche' e' l'unico posto che puo' garantirle:
	 * - `Destroyed` e' TERMINALE: un ponte abbattuto non si riattiva;
	 * - portare allo stato in cui l'arco gia' e' non e' un cambio (nessuna revisione, nessuna voce).
	 */
	static TArray<FRTArcChange> SetArcState(URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
		ERTHexArcState State);

	/**
	 * Scala l'integrita' dell'arco fra le due celle e lo porta a `Destroyed` quando arriva a zero. Il danno si
	 * applica a fase CONCLUSA, come per le coperture: l'ordine dei colpi non cambia l'esito.
	 *
	 * L'arco e' identificato dalla COPPIA di celle perche' la pianificazione non ha un bersaglio-arco — e' la
	 * stessa convenzione che `Action.ModifyArc` usa dal CP 8.5, e resta un limite dichiarato finche' l'HUD di
	 * E11 non porta un targeting vero.
	 */
	static TArray<FRTArcChange> DamageArc(URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To,
		int32 Amount);
};
