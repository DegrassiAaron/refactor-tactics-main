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

	/** Quanti layer separano i due estremi: `0` sullo stesso piano, `1` fra piani adiacenti. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static int32 TransitionLayerSpan(const FRTCellId& From, const FRTCellId& To);

	/**
	 * Una SCALA collega solo layer ADIACENTI (#1869). In **v0.1** la regola riguarda `Stair` e nessun altro
	 * `Kind`: gli altri cinque passano senza vincolo.
	 *
	 * ⚠️ **La regola e' `v0.1`, non permanente, e l'innesco della revisione e' NOMINATO** invece di essere
	 * lasciato al ricordo: **il giorno in cui una rampa o un ascensore vogliono saltare un piano**, questa
	 * funzione smette di poter dare una risposta sola e diventa una per `Kind` — un ascensore che serve il
	 * piano terra e il terzo e' esattamente il caso che la v0.1 non sa esprimere, e non e' un caso strano.
	 * Non vincolare adesso gli altri cinque e' deliberato: sarebbe inventare cinque regole che nessuna issue
	 * ha chiesto. E' la stessa forma del limite dichiarato in `DamageArc` qui sopra.
	 *
	 * 🔴 **E' illegale il SALTO, non la coincidenza — e la differenza e' misurata, non stilistica.** La #1869
	 * scrive la regola come `|Layer(To) - Layer(From)| == 1`, che vieterebbe anche lo span **zero**. Ma
	 * `FRTHexEdge::Kind` vale `Stair` per **default** (`RTHexCellData.h`), quindi ogni transizione scritta
	 * senza scegliere un tipo *e'* una scala, comprese quelle sullo stesso piano: con `== 1` il test
	 * `RefactorTactics.Map.Dependency.CellTakesTransitionsCitingIt` diventerebbe rosso — costruisce tre transizioni
	 * tutte a layer `0` e asserisce `ValidateMap().Num() == 0`. Il difetto che la issue descrive e' un altro,
	 * e lo dice lei stessa: *«nessuna regola vieta `L0 <-> L2`»*, *«scala che salta un layer»*. ∴ `>= 2`.
	 *
	 * Pura e senza mappa — le due celle portano il proprio layer. La chiamano **due** strati:
	 * `URTHexMapAsset::ValidateMap` sulla collezione e `URTHexArchTool` prima di committare. E' una funzione
	 * sola perche' due stesure della stessa soglia divergono, e divergerebbero in silenzio.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Hex")
	static bool IsTransitionLayerSpanLegal(const FRTCellId& From, const FRTCellId& To,
		ERTHexTransitionKind Kind);
};
