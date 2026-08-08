#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTIconCatalogData.generated.h"

class UTexture2D;

/**
 * Le dodici categorie semantiche del linguaggio iconografico (D-031).
 *
 * Sono DICHIARATE tutte e dodici fin dall'inizio e POPOLATE poche: la v0.1 riempie Identity, Action, Phase,
 * Status e Certainty (CP 20.2), le altre sette restano vuote finche' non esiste un sistema che le consumi.
 * Una categoria senza chiavi e' legale — e' la tassonomia; una CHIAVE senza asset non lo e'.
 *
 * Aggiungere valori solo IN CODA: i numeri gia' serializzati negli asset non cambiano.
 */
UENUM(BlueprintType)
enum class ERTIconCategory : uint8
{
	/** Chi e': personaggio, ruolo, fazione, relazione di squadra. */
	Identity,
	/** Cosa si sta per fare: le azioni del catalogo (`Action.Move`, `Action.Guard`, ...). */
	Action,
	/** In quale fase del turno: Prep, Dash, Blast, Move. Reaction NON e' una quinta fase. */
	Phase,
	/** Superfici e terreni: acqua, fuoco, elettricita', ghiaccio, fumo. */
	Environment,
	/** Oggetti manipolabili della mappa: porte, leve, ponti, ascensori. */
	MapInteraction,
	/** Stati temporanei di un'unita': i tag `Status.*` che il gioco applica davvero. */
	Status,
	/** Cosa la squadra SA: visibile, sentito, ultima posizione nota, area approssimativa. */
	Information,
	/** Ciclo di vita di una reazione: armata, opportunita', consumata, invalidata. */
	Reaction,
	/** Comunicazione fra alleati: ping, pronto, intento, conflitto. */
	Coordination,
	/** Quanto e' certo cio' che si vede: `Confirmed`, `Predicted`, `Uncertain`. */
	Certainty,
	/** Perche' l'azione non si puo' fare, o costa cara: fuoco amico, bersaglio invalido, ricarica. */
	Warning,
	/** Obiettivi: neutro, in cattura, conteso, posseduto, bloccato. */
	Objective
};

/**
 * Una voce del catalogo: una chiave semantica stabile e l'asset che la rappresenta.
 *
 * `IconId` ha la forma `UI.Icon.<Categoria>.<Nome>` e il segmento della categoria DEVE combaciare con
 * `Category`: e' cio' che rende la coerenza verificabile da una macchina invece che a occhio. Il prefisso
 * `UI.Icon.` non e' decorazione — senza, `Status.Wet` come icona e `Status.Wet` come Gameplay Tag sarebbero la
 * stessa stringa in un log, e D-031 li vuole concetti distinti.
 */
USTRUCT(BlueprintType)
struct FRTIconDef
{
	GENERATED_BODY()

	/** Chiave stabile (es. `UI.Icon.Status.Wet`). Rinominarla costa quanto rinominare un'azione a catalogo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Icons")
	FName IconId;

	/** Categoria dichiarata. Il validator la confronta col segmento dentro `IconId`. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Icons")
	ERTIconCategory Category = ERTIconCategory::Identity;

	/**
	 * La texture. Soft reference come il resto del progetto (`ARTUnit::TeamRingMaterial`): il catalogo si
	 * carica intero, le texture no.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Icons")
	TSoftObjectPtr<UTexture2D> Asset;

	FRTIconDef() = default;
	FRTIconDef(const FName& InIconId, ERTIconCategory InCategory, const TSoftObjectPtr<UTexture2D>& InAsset)
		: IconId(InIconId), Category(InCategory), Asset(InAsset) {}
};

/**
 * Esito di una risoluzione: l'asset e SE la chiave era davvero nel catalogo.
 *
 * Le due informazioni sono separate perche' il chiamante non deve dedurre il fallimento da un puntatore nullo:
 * `Asset` e' sempre utilizzabile (in caso di chiave sconosciuta e' il missing-icon), `bResolved` dice se e'
 * quello che era stato chiesto. Un widget che ignora `bResolved` mostra comunque qualcosa di visibile.
 */
USTRUCT(BlueprintType)
struct FRTIconResolution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Icons")
	TSoftObjectPtr<UTexture2D> Asset;

	/** Falso quando la chiave non era nel catalogo: `Asset` e' il missing-icon, non l'icona chiesta. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Icons")
	bool bResolved = false;

	FRTIconResolution() = default;
	FRTIconResolution(const TSoftObjectPtr<UTexture2D>& InAsset, bool bInResolved)
		: Asset(InAsset), bResolved(bInResolved) {}
};

/**
 * Il catalogo iconografico: chiave semantica -> asset (D-031, CP 20.1).
 *
 * Esiste perche' l'alternativa e' peggiore. Senza, ogni widget di E11 referenzierebbe la sua texture, e il
 * giorno in cui `Status.Wet` cambia disegno — o il giorno in cui serve una variante ad alto contrasto — la
 * modifica sarebbe un refactor di ogni widget invece di una riga di dato. La UI riceve una SEMANTICA e la
 * risolve qui: quello che il gameplay produce e' un `IconId`, mai un percorso di asset.
 *
 * Non e' una seconda fonte di verita' del gameplay: nessuna regola legge questo catalogo, e togliere
 * un'icona non cambia cosa succede in partita — cambia solo cosa si vede.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTIconCatalogData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Le voci del catalogo. L'ordine non conta: la ricerca e' per `IconId`. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Icons")
	TArray<FRTIconDef> Icons;

	/**
	 * L'icona restituita quando una chiave non si risolve. **Obbligatoria**: il validator rifiuta un catalogo
	 * che non ce l'ha, perche' senza di lei `ResolveIcon` non avrebbe niente da restituire e ricadremmo nel
	 * widget vuoto che questo checkpoint esiste per impedire.
	 *
	 * Non e' una catena di fallback per chiave: quella (`FallbackIconId` + controllo di ciclicita') e' CP 25.2.
	 * Un campo senza il suo controllo sarebbe un ciclo che scopre un giocatore invece della CI.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Icons")
	TSoftObjectPtr<UTexture2D> MissingIcon;

	/** ID primario stabile: `RTIconCatalog:<nome asset>`. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		static const FPrimaryAssetType Type(TEXT("RTIconCatalog"));
		return FPrimaryAssetId(Type, GetFName());
	}
};
