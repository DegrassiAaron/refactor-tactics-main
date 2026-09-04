#pragma once

#include "CoreMinimal.h"
#include "Core/RTTypes.h"
#include "Perception/RTTeamKnowledge.h" // FRTKnowledgeVerdict: il verdetto congelato di [D-223], come in `RTTurnLog.h`
#include "Ability/RTActionData.h" // ERTAbilityShape: la forma e' quella del catalogo, non una copia
#include "Turn/RTTurnRules.h"
#include "RTResolvedEvent.generated.h"

/** Tipo di evento risolto, per la riproduzione temporizzata (playback) del turno. */
UENUM(BlueprintType)
enum class ERTResolvedEventType : uint8
{
	Move,        // un'unita' ha percorso un path (Path = start + celle attraversate)
	Attack,      // un colpo risolto (Source -> Target, Amount = danno effettivo)
	HazardDamage,// danno da terreno (attraversamento o fine turno)
	Defeated,    // rimozione visiva di un'unita' eliminata

	/**
	 * L'impronta a terra di UN attacco: le celle che il resolver ha davvero investito, e la forma che le ha
	 * prodotte ([D-301]). Una voce per INTENTO aggressivo, non per vittima.
	 *
	 * 🔑 **Perche' non e' un campo di `Attack`.** `Attack` nasce nel loop per vittima: un'area su tre
	 * bersagli lo emette tre volte, e chi consuma dovrebbe deduplicare — logica nella presentazione, che
	 * [D-278] vieta. E soprattutto un'area che investe solo celle VUOTE non produce alcun `Attack`, quindi
	 * un campo li' sopra non arriverebbe mai: e' il caso che questo valore esiste per far esistere.
	 *
	 * ⚠️ **In CODA e non accanto ad `Attack`**: e' un `uint8` esposto a Blueprint, e inserirlo in mezzo
	 * rinumererebbe `HazardDamage` e `Defeated`, cambiando in silenzio ogni default gia' serializzato.
	 */
	AttackFootprint,

	/**
	 * Una reazione dichiarata è **scattata e si è risolta** (#2191): il momento in cui la difesa ha agito.
	 *
	 * 🔴 **Il momento non esisteva, e senza di esso due voci PIE non erano osservabili.** `PIE-VIS-DEFLECT`
	 * lo dice per intero: senza questo evento *«la parata non ha un momento proprio: resta la sola barra che
	 * scende poco»* — cioè esattamente l'*«attacco debole invece di una difesa riuscita»* che quella voce
	 * esiste per escludere. E `PIE-VIS-INTERPOSE` chiede che *«entrambe le metà si vedano»* — il colpo che
	 * parte verso uno e finisce sull'altro — dove la seconda metà era *«precisamente il momento che non
	 * esiste»*.
	 *
	 * ⚠️ **Non è un `Attack`, e la differenza è il soggetto.** `Attack` dice *«questo colpo ha tolto N a
	 * quello»*; qui il fatto è *«questa unità ha reagito»*, e il colpo che ne consegue — contrattacco,
	 * riduzione, scudo — resta un evento suo. Un solo `Attack` con un flag non basterebbe: una
	 * `DamageReduction` non produce nessun `Attack`, e sarebbe il caso più importante a restare invisibile.
	 *
	 * 🔑 **`SourceStableUnitId` è CHI reagisce; `TargetStableUnitId` è chi ha innescato**, cioè il verso
	 * naturale della frase «X reagisce a Y». ⛔ Non il bersaglio del contrattacco: quello lo porta l'`Attack`
	 * che segue, e duplicarlo qui creerebbe due fonti per lo stesso fatto.
	 *
	 * ⚠️ **In CODA, come `AttackFootprint`, e per la stessa ragione dichiarata sopra**: è un `uint8` esposto
	 * a Blueprint, e inserirlo in mezzo rinumererebbe i valori successivi cambiando in silenzio ogni default
	 * già serializzato.
	 *
	 * ⛔ **Che ASPETTO abbia non è deciso qui**: questo valore consegna il **momento**, non il suo VFX. La
	 * grammatica visiva della reazione si decide quando c'è qualcosa da animare — ed è fuori dallo scope di
	 * #2191, che lo dichiara.
	 */
	ReactionResolved
};

/**
 * Evento gia' risolto dalla logica, emesso a lock-in per essere RIPRODOTTO nel tempo.
 * L'animazione legge questi eventi: non decide nulla (invariante #1).
 *
 * 🔑 **E' un VALUE TYPE, e i soggetti sono id e non puntatori.** Fino a #1800 i due campi erano
 * `TWeakObjectPtr<ARTUnit>`: il significato del fatto dipendeva dalla vita di un Actor, proprio nel punto
 * in cui la presentazione deve leggere qualcosa di **gia' accaduto**. Un evento che porta due id si
 * confronta, si serializza e si asserisce **senza mondo** — ed e' la stessa scelta gia' fatta da
 * `FRTMoveRoute` nel TurnLog, che porta `StableUnitId` e nessun puntatore.
 *
 * ⚠️ **`0` non e' un'unita'** ([D-063]): `EnsureMatchRoster` assegna gli id **a partire da 1** e lascia lo
 * `0` libero apposta per dire «nessuna unita' dichiarata». Un `Defeated` ha quindi `TargetUnitId == 0`, e
 * un evento nato prima che il roster fosse congelato porta `0` anche in `SourceUnitId`: chi lo consuma
 * deve trattarlo come «nessuno», non come «l'unita' numero zero».
 *
 * 🔴 **Chi anima risolve, chi risolve non anima.** `ARTTurnManager::UnitByStableId` e' la porta che
 * ritrasforma l'id in `ARTUnit*`, e va usata **solo** quando c'e' davvero da muovere un cilindro o da
 * far partire un montage. Se l'unita' e' stata distrutta nel frattempo la porta risponde `nullptr`, che
 * e' esattamente cio' che rispondeva `TWeakObjectPtr::Get()` — il comportamento del playback non cambia,
 * cambia dove sta il puntatore.
 */
USTRUCT(BlueprintType)
struct FRTResolvedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	ERTMatchPhase Phase = ERTMatchPhase::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	ERTResolvedEventType Type = ERTResolvedEventType::Move;

	/**
	 * Chi ha agito, come `ARTUnit::StableUnitId`. `0` = nessun soggetto dichiarato.
	 *
	 * ⛔ **`StableUnitId` nel nome, e non `SourceUnitId` soltanto**: nel progetto esiste gia' una famiglia
	 * di campi chiamati `SourceUnitId` — `FRTActionInstance`, `FRTActionEvent`, `FRTNoiseEvent` — e
	 * `RTActionQueue.h` dichiara che li' l'intero e' l'**indice nello snapshot**, con sentinella
	 * `INDEX_NONE`. Sono due identita' diverse con due sentinelle diverse: chiamarle allo stesso modo
	 * significherebbe che un giorno qualcuno assegna l'una all'altra e il compilatore non ha nulla da dire.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	int32 SourceStableUnitId = 0;

	/** Chi ha subito, come `ARTUnit::StableUnitId`. `0` = nessuno. Solo `Attack` lo valorizza. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	int32 TargetStableUnitId = 0;

	/** Per Move: la rotta in celle (start incluso + celle attraversate, in ordine). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	TArray<FRTCellId> Path;

	/**
	 * Chi puo' vedere il modello percorrere CIASCUNA cella di `Path` ([D-223], `#1525`). Parallelo a
	 * `Path`, stesso indice.
	 *
	 * 🔴 **Esiste perche' il playback e' la seconda meta' di un difetto di cui la prima e' gia' chiusa.**
	 * `FRTMoveRoute::CellVerdicts` nomina i due errori speculari che [D-223] esiste per chiudere: il
	 * **leak** (la polilinea entra nella nebbia) e la **contraddizione** (la traccia nascosta mentre il
	 * modello e' disegnato). `#1497` ha chiuso il primo troncando la traccia; il secondo restava vivo
	 * perche' questo evento portava la rotta **senza** il verdetto, e il modello la percorreva intera.
	 *
	 * ⚠️ **Non e' un secondo calcolo.** Il verdetto e' lo STESSO che `FreezeRouteVerdicts` congela per la
	 * traccia, due righe sopra il punto in cui questo evento viene costruito, sulla stessa `Route`: la
	 * riparazione e' stata copiarlo, non ricalcolarlo. Se un giorno divergessero, traccia e modello
	 * tornerebbero a raccontare frasi diverse sullo stesso movimento.
	 *
	 * ⚠️ **Vuoto significa «nessun verdetto», e si legge fail-closed.** Chi consuma passa da
	 * `URTTeamKnowledgeLibrary::ObservedPrefixLength`, che su lunghezze disallineate risponde `0` invece
	 * di indovinare — mai indicizzando questo array direttamente.
	 *
	 * ⚠️ **`UPROPERTY()` nudo, non `BlueprintReadOnly`**: `FRTKnowledgeVerdict` non e' un tipo Blueprint, ed
	 * e' la stessa forma che `FRTMoveRoute::CellVerdicts` usa per lo stesso dato. Un verdetto leggibile da
	 * Blueprint sarebbe anche un verdetto **aggirabile** da Blueprint.
	 */
	UPROPERTY()
	TArray<FRTKnowledgeVerdict> CellVerdicts;

	/** Danno/scudo/durata secondo Type. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	int32 Amount = 0;

	// --- Solo per `AttackFootprint` ([D-301]). Vuoti/di default per ogni altro `Type`. ---

	/**
	 * Le celle investite, **nell'ordine che `HexHitCells` produce** (`URTHexLibrary::StableLess`).
	 *
	 * 🔴 **Copiate, non ricalcolate.** Sono le stesse che il resolver ha gia' usato per decidere chi
	 * colpire (`URTHexCombatLibrary::BuildHexBlastPlan`): chi le consuma **non deve chiamare
	 * `HexHitCells`**, o sarebbe una seconda implementazione di una primitiva canonica dentro la
	 * presentazione.
	 *
	 * ⚠️ **Non e' un `Path` e non ha un verso.** `Path` e' una rotta ordinata con un compagno indicizzato
	 * (`CellVerdicts`); questo e' un INSIEME. Riusare `Path` avrebbe legato un insieme a un array parallelo
	 * che per lui non significa niente.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	TArray<FRTCellId> HitCells;

	/**
	 * La forma che ha prodotto `HitCells`. Dichiarata e non dedotta: un `Single` resta distinguibile da
	 * un'`Area` di raggio 0, che a valle sono due disegni diversi con lo stesso numero di celle.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	ERTAbilityShape Shape = ERTAbilityShape::Single;

	/**
	 * Da dove il colpo e' partito, al momento in cui il resolver ha calcolato l'area.
	 *
	 * ⚠️ **Non si deriva dall'Actor.** Al playback l'unita' puo' essersi gia' mossa, e `ARTUnit::Cell`
	 * risponderebbe con la posizione finale del turno invece che con quella del colpo. E' la stessa
	 * ragione per cui `FRTBlastPreview` porta un `Origin` proprio.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	FRTCellId Origin;

	/**
	 * La cella MIRATA. E' l'unico dato che nessun altro campo puo' surrogare: con un'area su celle vuote
	 * `TargetStableUnitId` vale `0`, e senza questo non si saprebbe dove il colpo puntava.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Playback")
	FRTCellId AimCell;

	FRTResolvedEvent() = default;
};
