#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Map/RTCellId.h"
#include "Map/RTHexCellData.h"
// `FRTGeometrySegment` per i muri interni (#712). ⚠️ L'inclusione va in QUESTA direzione e non nell'altra:
// `RTGeometryGrammar.h` include gia' `RTHexCellData.h`, quindi mettere un segmento dentro la cella
// chiuderebbe un ciclo. E' il motivo per cui i muri interni vivono sull'ASSET e non sulla cella.
#include "Map/RTGeometryGrammar.h"
#include "RTHexMapAsset.generated.h"

/**
 * UN MURO CHE NON GIACE SU NESSUN BORDO.
 *
 * ⚠️ Esiste perche' la copertura non basta, e la misura che lo dimostra e' di `#712` / seduta `U22`: una
 * retta che taglia l'esagono passando per **due vertici opposti** attraversa una cella su tre per il
 * centro, e nelle altre due giace sul confine. Tracciata sulla griglia e' quindi per due terzi un bordo —
 * rappresentabile come `FRTHexCover` — e per un terzo una corda che non lo e'. Prima di questo tipo quel
 * terzo veniva calcolato, disegnato come anteprima, e poi **buttato via in silenzio**.
 *
 * 🔑 **ENTRA in `ComputeHash` dal 2026-09-01** (`#1830`), e prima non ci entrava — la riga che lo teneva
 * fuori diceva: *«il movimento e' cella-a-cella e un muro che sta dentro una cella non ne blocca nessuno […]
 * il giorno in cui un muro interno dovra' bloccare la linea di vista, quella e' una decisione di gioco e va
 * scritta come tale — e allora, ma solo allora, questo tipo entrera' nell'hash»*.
 *
 * Quel giorno e' arrivato: `D-269` e' la decisione, `URTHexOcclusionLibrary` il consumatore, e la vista di
 * `URTHexVisionLibrary` insieme alla linea di `URTOffensiveActionLibrary` leggono questi segmenti. Il
 * criterio non e' cambiato — nell'hash entra cio' che puo' cambiare un ESITO — e' cambiato il fatto.
 *
 * ⛔ **`StableId` resta fuori**, unico campo: nessuno risolve un muro interno per nome a runtime. Vedi il
 * commento esteso in `ComputeHash` e quello sul campo qui sotto.
 */
USTRUCT(BlueprintType)
struct FRTHexInteriorWall
{
	GENERATED_BODY()

	/** La cella che lo contiene. Il segmento e' in coordinate LOCALI a questa cella, come nella grammatica. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexMap")
	FRTCellId Cell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexMap")
	FRTGeometrySegment Segment;

	/**
	 * Nome stabile del muro (formato v12, #1864), con la disciplina di `FRTHexDoor::StableId` (CP 23.3).
	 *
	 * 🔑 **Esiste perche' un muro interno SI SPOSTA.** La sua chiave naturale sarebbe `(Cell, Segment)`, e
	 * il move cambia il `Segment`: un handle derivato si romperebbe esattamente durante l'operazione a cui
	 * deve sopravvivere. La copertura non ha questo problema — la sua chiave e' `(Cell, Edge)`, unica per
	 * bordo per una regola che `ValidateMap` gia' applica — e infatti non prende un campo.
	 *
	 * ⛔ **NON entra in `ComputeHash`, ed e' l'unico campo di questa struttura a restarne fuori dopo `#1830`.**
	 * Gli altri ci sono entrati perche' decidono se e dove il muro occlude; un nome no.
	 *
	 * ⚠️ Qui il criterio DIVERGE da `FRTHexDoor::StableId`, che nell'hash invece ci entra (#986): un nome di
	 * porta ci entra perche' `FindDoorEdges` risolve **per nome**, e rinominarla cambia quale bordo si apre
	 * per chiunque la citi. Nessuno risolve un muro interno per nome a runtime — lo fa solo l'editor. La riga
	 * precedente prometteva che *«il giorno in cui un muro interno toccasse le regole, entrerebbero prima i
	 * suoi campi e poi il suo nome»*: i campi sono entrati, il nome no, perche' quel giorno ha confermato la
	 * prima meta' della frase e non la seconda — la risoluzione per nome resta cosa dell'editor.
	 *
	 * `NAME_None` = muro senza nome, che e' cio' che ogni muro scritto prima di questo campo diventa
	 * rileggendosi — ed e' esattamente cio' che quei muri gia' erano.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexMap")
	FName StableId;

	/**
	 * IL MURO SI PUO' SCAVALCARE — la traversata autorata di `E23.7` ([D-308], formato v14, `#1828`).
	 *
	 * 🔑 **La scavalcabilita' e' un DATO, non una conseguenza dell'altezza**, ed e' la clausola con cui
	 * `D-308` impedisce a `Low`/`High` — vocabolario di MITIGAZIONE ([D-271]) — di diventare per inerzia un
	 * vocabolario di TRAVERSABILITA'. Un muretto non e' scavalcabile perche' e' basso: lo e' se un autore
	 * l'ha disegnato tale.
	 *
	 * 🔴 **Perche' il campo sta QUI e non su un tipo nuovo.** `D-308` nomina il vault *«il produttore che il
	 * terzo valore di `ERTIntraCellTraversal` aspettava»*, ma lo definisce **fra celle adiacenti** — e quel
	 * valore vive DENTRO una cella, dove una transizione fra due celle non arriva. Il muro interno e' cio'
	 * che divide la cella in due regioni: e' il posto in cui autorizzarne l'attraversamento senza inventare
	 * un secondo tipo. Precisazione registrata in `spec-cover-placement-intra-hex.md` §6.
	 *
	 * ⛔ **Non introduce una sottocella e non tocca l'occupancy**: la capacita' resta **una** unita' per
	 * `FRTCellId`, che e' il divieto di [D-289] e non cambia. Scavalcare permette di *arrivare* all'altra
	 * faccia, non di essere in due posti.
	 *
	 * ⚠️ **Entra in `ComputeHash`** con lo stesso criterio degli altri campi del segmento: decide se si passa,
	 * quindi due mappe che si giocano diversamente non possono avere lo stesso hash.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexMap")
	bool bTraversable = false;

	FRTHexInteriorWall() = default;
	FRTHexInteriorWall(const FRTCellId& InCell, const FRTGeometrySegment& InSegment)
		: Cell(InCell), Segment(InSegment) {}
};

#if WITH_EDITOR
/** L'asset e' cambiato per una via che i chiamanti non controllano (undo/redo, editing dal suo editor). */
DECLARE_MULTICAST_DELEGATE(FRTHexMapAssetChanged);
#endif

/**
 * Classe di mappa (CP 19.1): per QUALE formato quella mappa e' stata disegnata.
 *
 * Non e' una difficolta' ne' un tema: e' la scala. Una mappa Skirmish ha le distanze del 2v2 e i suoi tempi di
 * attraversamento; metterci dentro un 3v3 Standard non e' «piu' difficile», e' un'altra partita.
 *
 * **La simulazione non ramifica su questo valore.** Nessun `if (MapClass == ...)` nel resolver: la classe serve
 * al VALIDATOR — che rifiuta l'accoppiata sbagliata prima dell'allestimento — e ai parametri che il formato
 * porta con se'. Il giorno in cui una regola dipendesse dalla classe, dipenderebbe da un parametro dichiarato
 * dal formato, non dall'enum.
 *
 * Aggiungere valori solo IN CODA: il valore viaggia serializzato nell'asset mappa.
 */
UENUM(BlueprintType)
enum class ERTMapClass : uint8
{
	/** Scala del 2v2: e' la classe del vertical slice v0.1. */
	Skirmish,
	/** Scala del 3v3 competitivo (v0.2, E24): baseline del formato Standard. */
	Standard,
	/** Scala delle mappe a obiettivi multipli e logistica (v0.4, E30). */
	Operations
};

/**
 * Asset AUTOREVOLE e serializzato di una mappa esagonale (formato dati, non decorazione visiva).
 * Le celle sono conservate in un array con ORDINE STABILE (Layer, X, Y); una cache Id->indice velocizza l'accesso
 * runtime senza essere il formato autorevole. Nessun Actor per cella. Coerente con gli invarianti (determinismo).
 */
/**
 * Una sorgente comanda N bersagli (CP 23.4, #833). Cardinalita' dichiarata: `1->1` e `1->N`.
 *
 * ⚠️ **Lo schema regge `N->M` anche se la semantica e' `1->N`, ed e' deliberato**: `INT-5` — la composizione
 * di piu' sorgenti sullo stesso bersaglio (AND/OR/priorita') — e' aperta in `OPEN_DECISIONS.md`. Il dato puo'
 * quindi *rappresentare* due sorgenti che nominano lo stesso bersaglio, e la validazione lo **accetta**; a
 * rifiutarlo e' la risoluzione, con un reason code esplicito. Se lo rifiutasse la validazione, `INT-5`
 * riceverebbe una risposta dall'implementazione invece che da una decisione.
 */
USTRUCT(BlueprintType)
struct FRTInteractionBinding
{
	GENERATED_BODY()

	/** StableId (CP 23.3) della struttura che COMANDA. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	FName SourceId;

	/**
	 * StableId dei bersagli, **nell'ordine di applicazione**. L'ordine e' quello scritto qui: esplicito,
	 * deterministico, e non l'ordine di iterazione di una `TMap` (invariante n. 3).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<FName> TargetIds;

	FRTInteractionBinding() = default;
	FRTInteractionBinding(FName InSourceId, const TArray<FName>& InTargetIds)
		: SourceId(InSourceId), TargetIds(InTargetIds) {}
};

/**
 * PERCHE' UNA SEGNALAZIONE DI `ValidateMap` E' STATA PRODOTTA — `E23`, [D-289], `#1832`.
 *
 * 🔑 **Un reason code e non una stringa libera**, ed e' la disciplina che `ERTGeometryViolation`,
 * `ERTHexWaypointReason` e `ERTActionInvalidReason` gia' applicano: un test che distingue due regole
 * confrontando il **testo** del messaggio si rompe alla prima riformulazione, e allora chi riformula
 * impara a non toccare i messaggi — che e' il verso sbagliato in cui far pendere un validator che deve
 * essere leggibile da chi disegna.
 *
 * ⚠️ **Copre solo le regole di `#1832`.** Le segnalazioni piu' vecchie di `ValidateMap` — cella duplicata,
 * costo negativo, copertura a integrita' zero, transizione ridondante — restano righe testuali senza
 * codice: darglielo adesso significherebbe classificarne una ventina in una issue che non le possiede.
 */
UENUM()
enum class ERTMapValidationReason : uint8
{
	/** Nessuna regola di `#1832`: e' una segnalazione testuale storica. */
	None,

	/**
	 * REGOLA 1 — la cella non ha alcuna regione libera dove un'unita' possa stare, e **non** e' marcata
	 * `bBlocksMovement`. E' dato che si contraddice: il gioco la offrira' come raggiungibile e poi non
	 * saprebbe dove metterci chi ci arriva.
	 */
	NoLegalPlacement,

	/**
	 * REGOLA 2 — una copertura autorata su un bordo che **nessuna** regione di posa tocca: non produce
	 * alcuna `FRTCoverOption`, quindi nessuna unita' potra' mai usarla. Non e' illegale, e' inerte.
	 */
	UnreachableCover,

	/**
	 * REGOLA 4 — `bBlocksMovement` **derivato** (`bMovementBlockGenerated`) che la geometria attuale non
	 * giustifica piu': la cottura lo tolse o lo mise per una forma che non c'e' piu'.
	 *
	 * ⛔ **Non riguarda un blocco dipinto a mano.** *«L'autore vince»* e' gia' la regola di
	 * `DeriveStandability`, e segnalarlo qui la contraddirebbe.
	 */
	StaleGeneratedBlock,

	/**
	 * REGOLA 5 — due muri interni della stessa cella con lo stesso `FRTCoverSourceId`.
	 * `ERTGeometryViolation::DuplicateSegment` lo rifiuta **a monte**, ma una collezione ricostruita —
	 * migrazione, merge, incolla — puo' reintrodurlo.
	 */
	DuplicateCoverSource
};

/**
 * UNA SEGNALAZIONE DI `ValidateMap`, con il suo codice e la cella che la produce.
 *
 * ⛔ **Non blocca niente**: `ValidateMap` segnala, e il rifiuto immediato del gesto e' di `ValidateSegment`.
 * E' la divisione a due strati di `#620`, e questa struttura non la cambia.
 */
USTRUCT()
struct FRTMapValidationIssue
{
	GENERATED_BODY()

	UPROPERTY()
	ERTMapValidationReason Reason = ERTMapValidationReason::None;

	/** Le coordinate assiali della cella colpevole: un errore che non dice DOVE costringe a cercarlo a mano. */
	UPROPERTY()
	FRTCellId Cell;

	/** `false` = `Warning`: lo stato e' legale e risolto, ma quasi certamente non e' quello voluto. */
	UPROPERTY()
	bool bIsError = true;

	UPROPERTY()
	FString Message;
};

UCLASS(BlueprintType)
class REFACTORTACTICS_API URTHexMapAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Identita' stabile della mappa (per hash/replay/asset reference). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	FGuid MapId;

	/**
	 * Versione corrente del formato dati.
	 * v2: le transizioni entrano nell'hash + campo `Kind` sugli archi.
	 * v3 (CP 9.1): coperture per bordo di cella (`FRTHexCellData::Covers`).
	 * v4 (CP 9.3): porte per bordo di cella (`FRTHexCellData::Doors`).
	 * v5 (CP 9.4): stato, integrita' e conduttivita' sugli archi (`FRTHexEdge`).
	 * v6 (CP 19.1): classe di mappa (`MapClass`). Nessun dato esistente cambia significato: una mappa scritta
	 *     prima e' `Skirmish`, che e' la classe del vertical slice, cioe' cio' che quelle mappe gia' erano.
	 * v7 (#619): sovrapprezzo di occupazione sulla cella (`FRTHexCellData::OccupancySurcharge`). Nessun dato
	 *     esistente cambia significato: una mappa scritta prima non ha geometria cotta, quindi il suo
	 *     sovrapprezzo e' zero — che e' il default del campo, ed e' cio' che quelle mappe gia' erano.
	 * v8 (#621): provenienza della copertura (`FRTHexCover::bGenerated`). Nessun dato esistente cambia
	 *     significato: una mappa scritta prima non ha coperture cotte, quindi ogni sua copertura e' dipinta
	 *     a mano — che e' il default `false`, ed e' cio' che quelle coperture gia' erano.
	 * v9 (#832, CP 23.3): identita' stabile delle strutture (`FRTHexDoor::StableId`,
	 *     `FRTHexEdge::StableId`). Nessun dato esistente cambia significato: una mappa scritta prima non
	 *     nomina le proprie strutture, quindi ogni sua porta e ogni suo arco sono anonimi — che e' il
	 *     default `NAME_None`, ed e' cio' che quelle strutture gia' erano. `DoorId` non cambia mestiere:
	 *     resta l'indice di gruppo interno all'asset.
	 * v11 (#75, CP 10.2): obiettivo contendibile sulla cella (`FRTHexCellData::bIsObjective`). Nessun dato
	 *     esistente cambia significato: una mappa scritta prima non dichiarava obiettivi, quindi nessuna sua
	 *     cella ne e' uno — che e' il default `false`, ed e' cio' che quelle mappe gia' erano.
	 * v12 (#1864): nome stabile del muro interno (`FRTHexInteriorWall::StableId`). Nessun dato esistente
	 *     cambia significato: un muro scritto prima non ha un nome — default `NAME_None` — ed e' cio' che
	 *     quei muri gia' erano. ⚠️ Nasce da un'OPERAZIONE e non dalla simmetria con le porte: il muro
	 *     interno si sposta, e il move cambia la sua chiave naturale `(Cell, Segment)`. La copertura non
	 *     prende un campo perche' la sua chiave `(Cell, Edge)` e' unica per una regola gia' validata.
	 *
	 * v14 (#1828, `E23.7`): la traversata autorata sul muro interno (`FRTHexInteriorWall::bTraversable`).
	 *     Nessun dato esistente cambia significato: un muro scritto prima non e' scavalcabile — default
	 *     `false` — ed e' cio' che quei muri gia' erano, perche' fino a `D-308` la scavalcabilita' non
	 *     esisteva come dato. ⚠️ Il campo NON deriva dall'altezza: `Low` non diventa scavalcabile per
	 *     effetto di questo passo, ed e' precisamente cio' che `D-308` vieta.
	 * ⚠️ **Questo numero non viaggia da solo**: la sua storia e' qui, ma il valore che un asset porta nei
	 * propri byte e' `FRTHexMapCustomVersion` (#687, D-137). Alzarlo senza aggiungere il valore
	 * corrispondente all'enum non compila — e' voluto, vedi lo `static_assert` in `RTHexMapAsset.cpp`.
	 *
	 * Tutti i passi v1->v14 sono DICHIARATIVI: il campo nuovo nasce vuoto, nessun dato esistente cambia
	 * significato. Il primo passo TRASFORMATIVO e' ora eseguibile — prima di #687 non lo era, perche' la
	 * migrazione non partiva — ma resta il punto piu' delicato del formato: si scrive un
	 * `if (FormatVersion < N)` per volta, in ordine, e lo si prova su un asset serializzato.
	 */
	static constexpr int32 CurrentFormatVersion = 14;

	/**
	 * Versione del formato con cui l'asset e' stato scritto; `MigrateToCurrentFormat` la porta avanti.
	 *
	 * ⚠️ **Questa property NON e' la fonte della versione, e non puo' esserlo** (#687, [D-137]): il suo
	 * default e' mobile, quindi la serializzazione delta la salta sempre e non finisce mai nei byte. La
	 * fonte e' `FRTHexMapCustomVersion`, che viaggia nel summary del package; `Serialize()` scrive qui
	 * quel valore in lettura, cosi' chi legge questo campo a runtime trova comunque la verita'.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	int32 FormatVersion = CurrentFormatVersion;

	/**
	 * Versione che i BYTE dichiaravano al caricamento, prima di qualunque migrazione.
	 *
	 * Transient e non serializzata: e' un'osservazione sul viaggio, non un dato della mappa. Serve a
	 * rendere la migrazione **verificabile** — senza, dopo `PostLoad` ogni asset risulta alla versione
	 * corrente e «migrato da 0» e' indistinguibile da «creduto gia' aggiornato», che e' esattamente il
	 * difetto che #687 ha scoperto restando invisibile per otto versioni.
	 *
	 * `INDEX_NONE` significa «mai caricato da un archivio»: un asset costruito con `NewObject` non passa
	 * dalla serializzazione e non ha una versione d'origine da dichiarare.
	 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	int32 LoadedFormatVersion = INDEX_NONE;

	/**
	 * Per quale formato questa mappa e' stata disegnata (CP 19.1).
	 *
	 * NON entra in `ComputeHash`: l'hash risponde alla domanda «la stessa geometria produce la stessa
	 * partita?», e la classe non tocca la geometria ne' il comportamento — cambiarla non cambia un solo esito
	 * di turno. Ci finisce indirettamente `FormatVersion`, com'e' gia' successo a v3, v4 e v5.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	ERTMapClass MapClass = ERTMapClass::Skirmish;

	/** Dimensione dell'esagono (cm), usata per axial<->world. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexMap")
	float HexSize = 150.f;

	/** Quota (cm) tra un layer e il successivo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HexMap")
	float LayerHeight = 250.f;

	/** Revisione: incrementata a ogni modifica strutturale (invalidazione cache/path). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	int32 Revision = 0;

	/** Celle serializzate, in ordine stabile (Layer, X, Y). Formato autorevole (non una sola TMap). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<FRTHexCellData> Cells;

	/**
	 * Muri che non giacciono su nessun bordo (formato v10, #712). Vedi `FRTHexInteriorWall`.
	 *
	 * ⚠️ **Vive qui e non dentro la cella**, e non e' una preferenza: `RTGeometryGrammar.h` include gia'
	 * `RTHexCellData.h`, quindi un `FRTGeometrySegment` dentro la cella chiuderebbe un ciclo di include.
	 *
	 * ⚠️ **Ci finisce SOLO cio' che nessuna copertura puo' rappresentare.** Un segmento che chiude almeno
	 * un bordo e' gia' descritto dalle sue coperture, e scriverlo anche qui creerebbe due verita' sullo
	 * stesso muro — che e' la classe di difetto che questa seduta ha passato la giornata a smontare.
	 * L'approssimazione dichiarata: un segmento che chiude un bordo **e** sporge dentro la cella e'
	 * rappresentato dalla sola copertura, e la sua parte interna non viene conservata.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<FRTHexInteriorWall> InteriorWalls;

	/** Transizioni esplicite (archi verticali/speciali): scale, rampe, ponti, tunnel, ascensori. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<FRTHexEdge> Transitions;

	/**
	 * Grafo di interazione `Source -> Target` (CP 23.4, #833): quale struttura comanda quali altre.
	 *
	 * E' un DATO d'asset e non codice: nessuna coppia `S1 -> D1` vive in C++ ne' in un riferimento Blueprint.
	 * Sorgente e bersagli si nominano con gli **StableId di CP 23.3** — questa feature non inventa un secondo
	 * sistema di identita', e infatti `ResolveInteractionTargets` passa da `FindDoorEdges`.
	 *
	 * ⚠️ L'array e' l'ordine AUTOREVOLE dei bersagli: `TargetIds` si applica come e' scritto, mai nell'ordine
	 * di iterazione di una `TMap`/`TSet` (invariante n. 3). Chi rieditasse l'asset cambierebbe l'ordine di
	 * applicazione, ed e' voluto: e' una scelta d'autore, non un dettaglio d'implementazione.
	 *
	 * ⚠️ **`N` non ha un tetto, ed e' dichiarato invece che lasciato scoprire.** Il DoD di #833 chiede l'uno o
	 * l'altro: qui non c'e' limite perche' non c'e' un fallimento MISURATO che ne motivi uno. Cio' che la scala
	 * poteva rompere e' l'ORDINE, e quello e' verificato — `InteractionGraph.OrderHoldsAtScale` prova quaranta
	 * bersagli e pretende la sequenza dichiarata. Un tetto scritto senza un fallimento che lo giustifichi
	 * rifiuterebbe asset legittimi in nome di una costante che nessuno sa spiegare.
	 *
	 * 🔴 **Il COSTO pero' non e' lineare, e questa riga lo diceva.** Affermava che «ne' la risoluzione ne'
	 * l'applicazione degradano con la cardinalita', essendo entrambe scansioni lineari»: falso, e trovato da una
	 * code review. `ApplyInteraction` chiama la commutazione **una volta per bersaglio**, e ogni chiamata
	 * scandisce l'intero array delle celle due volte (gruppo + commutazione), quindi il costo va come
	 * `bersagli × celle × porte`; la risoluzione paga `TargetIds × Bindings` per il controllo dei bersagli
	 * contesi, piu' una scansione completa per ogni `FindDoorEdges`. La decisione di non mettere un tetto resta,
	 * perche' poggia sull'assenza di un fallimento e non sul costo — ma il costo si dichiara invece di
	 * affermarne uno comodo e non misurato. Se un giorno una mappa grande lo rendera' visibile, il rimedio e'
	 * un indice, non un limite alla cardinalita' che un autore puo' scrivere.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HexMap")
	TArray<FRTInteractionBinding> InteractionBindings;

	/** Aggiunge o aggiorna (per Id) una cella; incrementa la revisione. */
	void AddOrUpdateCell(const FRTHexCellData& Cell);

	/**
	 * Aggiunge o aggiorna PIU' celle incrementando la revisione UNA SOLA VOLTA. Serve a chi compie una modifica
	 * topologica che tocca piu' celle ma e' un evento solo — un portone largo tre bordi si apre una volta, non
	 * tre — cosi' che chi osserva la revisione non veda tre cambi dove ce n'e' stato uno.
	 * Un array vuoto non incrementa nulla.
	 */
	void UpdateCells(const TArray<FRTHexCellData>& InCells);

	/** Inizia una pennellata: Modify() una volta (stato pre-pennellata per l'undo). Nessuna transazione (la apre il caller). */
	void BeginStroke();

	/**
	 * Dipinge una cella dentro una pennellata: crea/aggiorna. Vero se applicata. Niente Sort/Dirty/refresh.
	 * `InBlocksLineOfSight` non passato preserva il flag della vista (comportamento storico); passato lo scrive.
	 */
	bool PaintCellInStroke(const FRTCellId& Id, ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement,
		TOptional<bool> InBlocksLineOfSight = TOptional<bool>());

	/** Cancella una cella dentro una pennellata. Vero se esisteva ed è stata rimossa. Niente Sort/Dirty/refresh. */
	bool EraseCellInStroke(const FRTCellId& Id);

	/** Chiude la pennellata: SortCells + MarkPackageDirty. */
	void EndStroke();

	/**
	 * Logica pura del 'pennello': se Existing != nullptr parte da esso (PRESERVA sempre Height), altrimenti da
	 * una cella nuova con l'Id dato; applica Surface/MoveCost/bBlocksMovement. Non muta l'asset.
	 *
	 * `InBlocksLineOfSight` **non passato** preserva `bBlocksLineOfSight` — il comportamento storico, pinnato da
	 * `RefactorTactics.HexMap.ApplyBrushMerge`, e il default. **Passato**, lo scrive.
	 *
	 * L'asimmetria con `bBlocksMovement` (che si scrive sempre) nasceva dal fatto che nessuno strumento
	 * dell'editor sapeva impostare il flag della vista: preservarlo era l'unico modo di non perderlo per
	 * sempre. Da quando il pennello lo espone, chi vuole scriverlo lo chiede.
	 */
	static FRTHexCellData ApplyBrush(const FRTHexCellData* Existing, const FRTCellId& Id,
		ERTHexSurface Surface, int32 MoveCost, bool bBlocksMovement,
		TOptional<bool> InBlocksLineOfSight = TOptional<bool>());

	/** Rimuove la cella con l'Id dato; vero se esisteva. */
	bool RemoveCell(const FRTCellId& Id);

	/**
	 * Svuota celle **e** transizioni, incrementando la revisione **una volta sola**; vero se c'era
	 * qualcosa da togliere.
	 *
	 * Esiste perche' `ARTHexMapActor::ClearAsset` scriveva sui due array direttamente, ed era l'unica
	 * modifica strutturale dell'asset a non muovere `Revision` — proprio la piu' grande possibile. Un
	 * percorso calcolato prima restava valido dopo, e due voci di TurnLog ai due lati di uno svuotamento
	 * portavano la stessa `GraphRevision`, che e' cio' che quel campo esiste per rendere impossibile.
	 *
	 * La revisione e' responsabilita' del DATO, non di chi lo modifica: per questo il reset vive qui e
	 * l'actor lo chiama, come per ogni altra modifica.
	 */
	bool ClearAll();

	/**
	 * **Sostituisce** celle e transizioni con quelle date, incrementando la revisione **una volta sola**;
	 * vero se dopo la chiamata l'asset contiene qualcosa.
	 *
	 * E' il caso che alla famiglia mancava: `AddOrUpdateCell` e' la singola, `UpdateCells` il gruppo,
	 * `ClearAll` lo svuotamento — e «rimpiazza tutto» finiva scritto a mano dai chiamanti, con un
	 * `AddOrUpdateCell` per cella. Generare l'arena della v0.1 muoveva cosi' la revisione **98 volte**
	 * per un evento solo, e le transizioni — assegnate direttamente — per niente.
	 *
	 * Vale la regola che `UpdateCells` enuncia: *«un portone largo tre bordi si apre una volta, non tre»*.
	 * Rimpiazzare la mappa e' un evento, e chi osserva `Revision` per invalidare una cache deve vederne uno.
	 *
	 * Sostituire con contenuto **vuoto** e' uno svuotamento: delega a `ClearAll`, guardia inclusa.
	 */
	bool ReplaceContent(const TArray<FRTHexCellData>& InCells, const TArray<FRTHexEdge>& InTransitions);

	/** Puntatore alla cella con l'Id dato, o nullptr se assente. */
	const FRTHexCellData* FindCell(const FRTCellId& Id) const;

	/** Vero se la mappa contiene la cella. */
	bool ContainsCell(const FRTCellId& Id) const;

	int32 NumCells() const { return Cells.Num(); }

	/** Layer distinti presenti nelle celle, ordinati in modo crescente. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HexMap")
	TArray<int32> GetLayers() const;

	/** Id delle celle appartenenti al Layer indicato, in ordine stabile. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HexMap")
	TArray<FRTCellId> CellsInLayer(int32 Layer) const;

	/**
	 * Regione contigua (flood-fill) a partire da Start: visita i vicini dello STESSO layer che ESISTONO nell'asset
	 * e hanno la STESSA superficie di Start (frontiera a stack; l'ordine di visita non altera l'insieme risultante).
	 * Include Start. Se Start non esiste -> regione vuota. Pura/read-only/deterministica.
	 */
	TArray<FRTCellId> FloodRegion(const FRTCellId& Start) const;

	/**
	 * Aggiunge (o aggiorna, se From->To esiste gia') una transizione verticale/speciale; se bBidirectional aggiunge
	 * anche l'arco inverso To->From. Incrementa la revisione. Nessun arco duplicato per direzione.
	 */
	void AddTransition(const FRTCellId& From, const FRTCellId& To, int32 Cost = 1,
		ERTHexTransitionKind Kind = ERTHexTransitionKind::Stair, bool bBidirectional = true);

	/** Rimuove la transizione From->To (e, se bBothDirections, anche To->From). Vero se ne ha rimossa almeno una. */
	bool RemoveTransition(const FRTCellId& From, const FRTCellId& To, bool bBothDirections = true);

	/**
	 * Sostituisce gli archi indicati (per coppia From->To) e incrementa la revisione UNA SOLA VOLTA: un ponte
	 * bidirezionale sono due archi ma un evento solo, come il portone di CP 9.3. Un array vuoto non incrementa.
	 */
	void UpdateTransitions(const TArray<FRTHexEdge>& InEdges);

	/** Ordina le celle in modo stabile (Layer, X, Y) e invalida la cache. */
	void SortCells();

	/** Hash deterministico del contenuto delle celle (indipendente dall'ordine di inserimento). */
	uint32 ComputeHash() const;

	/**
	 * L'ORDINE CANONICO dei muri interni, per chi deve mescolarli in un hash.
	 *
	 * 🔑 **Esiste per non averne due.** `ComputeHash` e `URTMatchStateHashLibrary` mescolano entrambi la
	 * geometria intra-cella da `#1830`, e due ordinamenti scritti separatamente sono la forma esatta del
	 * difetto che `#986` ha gia' pagato su `bConductsElectricity`: un campo mescolato da uno e saltato
	 * dall'altro, *«due hash che divergevano senza che nessuno l'avesse deciso»*.
	 *
	 * L'ordine dell'array lo decide chi edita l'asset, quindi non e' dato: si ordina per cella e poi per
	 * giacitura. Gli estremi contano come coppia NON ordinata — e' lo stesso segmento anche percorso al
	 * contrario, ed e' gia' la regola del suo `operator==`.
	 */
	static void SortInteriorWallsCanonically(TArray<FRTHexInteriorWall>& Walls);

	/**
	 * La mappa dichiara almeno una cella OBIETTIVO (formato v11, CP 10.2)?
	 *
	 * ⚠️ Esiste perche' il Cleanup deve poter TACERE: su una mappa senza obiettivi non si scrive nessuna voce
	 * di categoria `Objective`, altrimenti ogni partita gia' archiviata guadagnerebbe una voce per turno che
	 * dice «non e' successo niente» — e il corpus golden divergerebbe su mappe che non hanno obiettivi.
	 */
	bool HasObjectiveCell() const;

	/**
	 * La cella OBIETTIVO in ordine STABILE, o un id invalido se la mappa non ne dichiara nessuna.
	 *
	 * ⚠️ **Stabile e non «la prima dell'array»**: l'ordine di `Cells` lo decide chi edita l'asset, quindi
	 * spostare due voci nel dettaglio cambierebbe quale cella il TurnLog nomina — a mappa identica. E' lo
	 * stesso criterio con cui `ComputeHash` ordina prima di mescolare.
	 *
	 * Con piu' obiettivi (CP 31.1, post-v0.1) questo accessore non basta piu', ed e' voluto: chi li
	 * introduce deve toccare i suoi chiamanti invece di ereditare in silenzio «il primo».
	 */
	FRTCellId FirstObjectiveCell() const;

	/**
	 * Cella "centrale" della mappa: mediana del bounding box assiale delle celle del layer piu' basso.
	 * Serve a inquadrare la mappa (camera) senza assumere che sia centrata sull'origine — una mappa
	 * disegnata nell'editor puo' stare tutta altrove. Mappa vuota -> (0,0,0). Pura e deterministica.
	 */
	FRTCellId GetCenterCell() const;

	/**
	 * Validazione minimale: Id duplicati, costi negativi, transizioni verso celle inesistenti. Ritorna gli
	 * errori come testo, compresi quelli tipizzati di `ValidateMapDetailed` gia' formattati.
	 *
	 * ⛔ **Segnala, non blocca**, ed e' la meta' che non cambia: il rifiuto immediato di un gesto e' di
	 * `URTGeometryGrammarLibrary::ValidateSegment`, e la correzione automatica di uno stato invalido e'
	 * vietata dal Decision Record.
	 */
	TArray<FString> ValidateMap() const;

	/**
	 * LE REGOLE DI TOPOLOGIA DI `#1832`, con reason code e cella — [D-289], `E23`.
	 *
	 * Quattro regole, e ognuna descrive un dato che si contraddice invece di uno che e' scritto male:
	 * `NoLegalPlacement`, `UnreachableCover`, `StaleGeneratedBlock`, `DuplicateCoverSource`.
	 *
	 * 🔑 **Elenco DETERMINISTICO e ordinato** per `StableLess(Cell)`, poi per reason, poi per indice della
	 * sorgente. Non si appoggia all'ordine di `Cells`, che lo decide chi edita l'asset: un elenco che
	 * cambia ordine fra due esecuzioni non e' verificabile, ed e' un criterio d'accettazione della issue.
	 *
	 * ⚠️ **La regola 3 della issue — «transizione di faccia illegale» — NON e' qui, e non e' una
	 * dimenticanza.** Il testo la definisce come *«una traversata autorata che collega due regioni
	 * attraversando geometria bloccante senza un'apertura»*, e la forma a due maschere scelta da `#1828`
	 * rende quella configurazione **irrappresentabile**: se fra le due regioni ci fosse anche un muro non
	 * scavalcabile, la maschera `Traversable` resterebbe separata e la risposta sarebbe `Blocked`. Una
	 * regola per essa sarebbe codice morto, e il suo test non potrebbe fallire. Vedi
	 * `URTHexCoverPlacementLibrary::ClassifyIntraCellTraversalWithAuthored`.
	 *
	 * ⚠️ **E la regola 6 — «aspettativa di due occupanti» — manca del proprio antecedente**: nessun campo
	 * esprime quante unita' il livello si aspetti in una cella, e dedurlo dalla forma della cella sarebbe
	 * l'opposto di cio' che un validator fa. Ha una issue propria.
	 */
	void ValidateMapDetailed(TArray<FRTMapValidationIssue>& OutIssues) const;

	/**
	 * Porta l'asset alla versione corrente del formato. Idempotente (`PostLoad` puo' ripetersi) e conservativa:
	 * v2 -> v3 non converte nulla, perche' il campo `Covers` nasce vuoto — quello che deve dimostrare e' di
	 * NON toccare i dati esistenti.
	 */
	void MigrateToCurrentFormat();

	/** Migra l'asset appena caricato: un asset scritto con un formato vecchio non deve mai arrivare al gioco. */
	virtual void PostLoad() override;

	/**
	 * Dichiara la versione di formato all'archivio e, in lettura, la RICAVA da li' invece che dalla property.
	 *
	 * E' il lato che mancava: senza, `FormatVersion` prende il default del CDO e ogni asset si crede gia'
	 * aggiornato. La migrazione resta in `PostLoad` — questo metodo non migra, stabilisce **da dove si
	 * parte**.
	 */
	virtual void Serialize(FArchive& Ar) override;

	/**
	 * Invalida la cache Id->indice. Va chiamata quando Cells viene modificato SENZA passare dalle API di questa
	 * classe (tipicamente un undo/redo, che riscrive la property direttamente): la cache resterebbe altrimenti
	 * allineata allo stato precedente e FindCell leggerebbe l'indice sbagliato — o fuori dall'array, se le celle
	 * sono diminuite.
	 */
	void InvalidateLookup() const { bLookupDirty = true; }

#if WITH_EDITOR
	/** Notifica di cambiamento non mediato dalle API (undo/redo): chi mostra l'asset deve riallinearsi. */
	FRTHexMapAssetChanged OnMapChanged;

	/** Invalida la cache e notifica gli osservatori: dopo un undo i dati sono cambiati sotto i piedi a tutti. */
	virtual void PostEditUndo() override;
#endif

private:
	/** Cache Id->indice (transient, ricostruita pigramente). Non e' il formato autorevole. */
	mutable TMap<FRTCellId, int32> Lookup;
	mutable bool bLookupDirty = true;
	void EnsureLookup() const;
};
