#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTHeroData.generated.h"

class URTActionData;

/**
 * Definizione data-driven di un EROE del catalogo v0.1 (Aevik, Phase, Riktor, Ivrin).
 *
 * Contiene solo cio' che il catalogo dichiara come **fisso** dell'eroe: identita', statistiche base e le
 * azioni fondamentali (attacco base compreso). Cio' che e' configurabile FRA eroi diversi (variante d'arma,
 * gadget, modulo di reazione) sta in `URTEquipmentData` e nel loadout; la variante di UNA abilita'
 * fondamentale invece e' dell'eroe stesso (`URTActionData::Variants`), perche' i suoi numeri dipendono
 * dall'abilita' che modifica — la stessa "Scarica ramificata" non avrebbe senso su un'abilita' di un altro
 * eroe.
 *
 * `Actions` ha **da cinque a sei** elementi nel catalogo v0.1: indice 0 l'attacco base (fascia dalla portata,
 * `URTCatalogLibrary::MakeBasicAttack`), indici 1-4 le quattro abilita' fondamentali — di cui **una sola**
 * dichiara varianti — e un indice 5 **facoltativo**, una generica del catalogo core portata nel kit.
 * `URTHeroCatalogLibrary::ValidateHeroes` fa valere l'intervallo e il vincolo della variante unica.
 *
 * ⚠️ **Era «ESATTAMENTE cinque», e la parola in maiuscolo non e' bastata a tenerlo vero.** Sul roster v0.1
 * ne hanno sei **Phase** (`Hero.Muiren.TideGuard`) e **Ivrin** (`Hero.Ivrin.PhaseGuard`), entrambe derivate
 * da `Action.Shield`; Aevik e Riktor restano a cinque. Il tetto e' 6 e non «quante ne vuoi» perche' oltre,
 * il kit supera le posizioni che l'input raggiunge — `PlayerInput.EveryKitEntryIsReachable` lo misura.
 * Il costo dell'intervallo e' dichiarato accanto al validator: non dice piu' «questo eroe e' completo».
 *
 * Solo interi (invariante #4). Riferimento: docs/balance/RT_HeroCatalog_v0.1.md
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTHeroData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** ID stabile dell'eroe (es. `Hero.Aevik`). Chiave del data asset: non cambia mai. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName HeroId;

	/** Nome mostrato (UI/log). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 MaxHealth = 1000;

	/** Punti movimento per turno (budget standard del catalogo: 5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 MovePoints = 5;

	/** Portata visiva in celle esagonali. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 VisionRange = 5;

	/**
	 * Soglia d'udito ([D-041](../../../docs/decisions/RT_PDR_00_Decision_Log.md)): il rumore ricevuto va
	 * **almeno** a questo valore perche' l'eroe lo senta. Scala `0-10`, la stessa dell'intensita'
	 * (`Wait 0 · Sprint 5 · Dash 6 · esplosione 10`), e soglia **bassa = orecchio fine**.
	 *
	 * Quarta statistica, e **compensa** la vista invece di seguirla: Aevik 5 · Phase 3 · Riktor 3 · Ivrin 5.
	 * Chi vede lontano sente meno. La terza via — udito allineato alla vista — e' stata scartata perche'
	 * raddoppiare lo stesso vantaggio su due canali renderebbe gli eroi da ricognizione *migliori*, non
	 * diversi.
	 *
	 * ⚠️ Il default e' volutamente `5` e non `0`: `0` significherebbe «sente qualunque cosa», cioe' il
	 * contrario di un valore neutro. Un eroe che si dimenticasse di dichiararla sentirebbe tutta la mappa.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 HearingThreshold = 5;

	/**
	 * Celle di spinta assorbite prima di essere spostato. **Soglia**, non riduzione (D-038).
	 *
	 * ⚠️ Il roster v0.1 e' interamente a `0` da D-075 (#402), Riktor compreso — che fino al 2026-08-10 era
	 * l'unico a `1`. A soglia 1, siccome ogni spinta del gioco vale 1, il campo comprava **immunita' totale**
	 * invece di stabilita'. La meccanica resta implementata (`RTTurnManager.cpp`, ramo `ERTActionEffect::Push`)
	 * e **dormiente**: si risveglia il giorno in cui una spinta >= 2 entra nel catalogo.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 PushResistance = 0;

	/**
	 * **Di quanto `Status.Guarded` riduce OGNI colpo valido dell'arco frontale** ([D-408]).
	 *
	 * 🔴 **Il valore lo dichiara il PERSONAGGIO, ed e' la meta' di `D-408` che tocca i dati.** Fino al
	 * 2026-09-20 il valore era UNICO per tutti, una costante condivisa, e la Guardia era
	 * un **pool** di assorbimento ([D-292]): un budget totale per il turno, che i colpi frontali
	 * consumavano. `D-408` ritira il pool e tiene la riduzione.
	 *
	 * ⚠️ **Cio' che cambia e' il TETTO, ed e' voluto**: un pool limita la mitigazione totale del turno, una
	 * riduzione per colpo no. La sorgente §13.8 lo riconosce e lo dichiara deliberato.
	 *
	 * ✅ **La proprieta' che `D-292` difendeva NON si perde**: una riduzione per colpo e'
	 * permutation-invariant **per costruzione**, perche' non c'e' un avanzo da perdere. La pinna
	 * `Combat.GuardReductionIsPermutationInvariant`.
	 *
	 * ⛔ **Il numero per ciascun eroe e' TARATURA e la possiede `balance/`** ([D-282]): qui c'e' il
	 * default di catalogo, ancora `URTCombatLibrary::GuardReductionDefault` (100 dal [D-602], uguale per tutti e
	 * quattro: nessun eroe ne dichiara uno proprio).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 GuardReduction = 100; // = URTCombatLibrary::GuardReductionDefault: l'header dei dati non include `Combat/`

	/**
	 * DEF del personaggio: la mitigazione moltiplicativa del danno DIRETTO, `100 / (100 + Defense)`
	 * ([D-589] (1), [D-595] (3); E53 CP 53.2b). La formula vive in `URTCombatLibrary::MitigateDirectDamage`.
	 *
	 * Valori dal CP 53.3c (D-595 (3), gli otto del GDD: Riktor 45, Steel 60, Kallari 15, Feng Mao 35, Murdock 20,
	 * Gadget 15, Muriel 25, The Fey 20; i quattro a runtime prendono quelli dello slot che diventeranno: Aevik 15,
	 * Muiren 25, Riktor 45, Ivrin 15). Fino a CP 53.3b era zero per tutti, cioe' l'identita': i campi sono entrati
	 * prima dei numeri. ⛔ Il numero per eroe e' TARATURA e la possiede `balance/` ([D-282]).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 Defense = 0;

	/**
	 * RES di default per i dodici elementi ([D-593]): la mitigazione del danno ELEMENTALE e AMBIENTALE con la
	 * stessa forma della DEF ([D-589] (1)). `ElementResistance` la sostituisce per il singolo elemento che vi
	 * compare; per gli altri vale questo numero ([D-595] (3): «la RES unica come default per i dodici»).
	 * Valori dal CP 53.3c: Aevik 35, Muiren 40, Riktor 35, Ivrin 20 (la RES del GDD dello slot). ⚠️ Nessun colpo
	 * porta ancora un elemento, quindi nessun chiamante la legge: e' un dato del catalogo in attesa del suo lettore.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 ResistanceDefault = 0;

	/**
	 * RES per singolo elemento, chiave `Element.<X>` fra i dodici di [D-593] (`Warp`, `Fire`, `Water`, `Earth`,
	 * `Electric`, `Frost`, `Decay`, `Life`, `Arcane`, `Aegis`, `Air`, `Disintegration`). Il validator rifiuta
	 * una chiave fuori da quella lista e un valore negativo. Vuota oggi: nessun eroe differenzia un elemento.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	TMap<FName, int32> ElementResistance;

	/**
	 * Bonus alle spalle dell'ATTACCANTE ([D-587] (2)(3)): si somma alla Base del colpo PRIMA della mitigazione
	 * quando il colpo arriva da uno dei tre lati posteriori del bersaglio (D-126), quindi la DEF lo riduce come
	 * il resto. Valori iniziali [D-599]: flanker 90, standard 60, tank 30 sulla scala ×10, nel catalogo dal
	 * CP 53.3c (Ivrin 90; Aevik e Muiren 60; Riktor 30). La fascia e' una convenzione di scrittura, non un campo.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	int32 RearBonus = 0;

	/**
	 * Step di rotazione spendibili a FINE movimento a budget — la famiglia `Move` di ADR-0008 §1.
	 *
	 * | Personaggio | `MoveEndPivotMaxSteps` | `DashEndPivotMaxSteps` |
	 * |---|---:|---:|
	 * | Aevik | 2 | 2 |
	 * | Phase | 2 | 3 |
	 * | Riktor | 1 | **0** |
	 * | Ivrin | 3 | 3 |
	 *
	 * ⚠️ **Ipotesi iniziali, non bilanciamento approvato.** L'ADR li dichiara «*ipotesi da scenario/playtest*»
	 * e chiede di non canonizzarli come definitivi: entrano nel catalogo perche' il modello ha bisogno di un
	 * valore per esistere, non perche' siano stati misurati. La taratura e' lavoro separato (`#1605` §Out of
	 * scope), e il trigger di revisione e' in fondo ad ADR-0008.
	 *
	 * 🔵 I default `1` e `0` sono ADR-0005 nella nuova unita' di misura, non zeri di comodo: un eroe che si
	 * dimenticasse di dichiararli si comporterebbe come il gioco si comportava prima di ADR-0008, non come
	 * una statua. Vedi `FRTPivotBudget` in `RTActionDef.h`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog", meta = (ClampMin = "0", ClampMax = "3"))
	int32 MoveEndPivotMaxSteps = 1;

	/** Step di rotazione spendibili a FINE mobilita' lineare — la famiglia `Dash` di ADR-0008 §1. Vedi sopra. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog", meta = (ClampMin = "0", ClampMax = "3"))
	int32 DashEndPivotMaxSteps = 0;

	/** Affinita' ambientale dichiarata (elettricita', acqua, strutture, movimento). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName Affinity;

	/**
	 * Debolezza dichiarata dell'eroe. Il PDF del catalogo la elenca fra gli elementi fissi ma non la esplicita
	 * per nessuno dei quattro eroi (docs/balance/RT_HeroCatalog_v0.1.md §5): **non si inventa**, va
	 * decisa esplicitamente per ciascun eroe. Un eroe senza debolezza non passa `ValidateHeroes`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName Weakness;

	/**
	 * Il **Reaction Profile** che questo eroe arma col `Brace` (E14.7, [D-047]). `None` = profilo base.
	 *
	 * ⚠️ E' un **riferimento**, non il profilo: il dato vive nel catalogo (`Profile.<Nome>`), e l'eroe lo
	 * nomina. E' la forma letterale di «profilo come dato, non ramo nel resolver» — con le risposte scritte
	 * qui, ogni eroe nuovo sarebbe una copia del contenuto invece che un puntatore a un'entita' condivisa, e
	 * riassegnare un profilo costerebbe un rename in due posti.
	 *
	 * 🔵 **`None` non e' un valore mancante**: e' Riktor, e per lui e' la scelta giusta — `Hold Ground` fa
	 * gia' cio' che un suo profilo avrebbe fatto, quindi la cardinalita' resta 1 e nessuna finestra si apre.
	 * Per questo il campo **non** e' fra quelli che `ValidateHeroes` pretende: un eroe senza profilo e'
	 * valido, a differenza di un eroe senza `Weakness`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	FName ReactionProfileId;

	/**
	 * Le azioni dell'eroe: attacco base (indice 0) + quattro abilita' fondamentali (indici 1-4) + una
	 * generica del catalogo core **facoltativa** (indice 5). Cardinalita' 5-6, vedi il docstring della classe.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Catalog")
	TArray<TObjectPtr<URTActionData>> Actions;

	/** ID primario stabile: `RTHero:<HeroId>`; senza HeroId ricade sul nome dell'asset. */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		static const FPrimaryAssetType Type(TEXT("RTHero"));
		return FPrimaryAssetId(Type, HeroId.IsNone() ? GetFName() : HeroId);
	}
};
