#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Perception/RTKnowledgeView.h" // FRTKnowledgeSubject: il soggetto contro cui il verdetto e' congelato
#include "Perception/RTTeamKnowledge.h"
#include "Turn/RTTurnLog.h"
#include "RTReplayAuditLibrary.generated.h"

/**
 * Versione del formato dell'artefatto d'audit.
 *
 * ⚠️ **E' la sua, e non quella del TurnLog.** Le due cose si versionano separatamente perche' cambiano per
 * ragioni separate: e' l'intera premessa di [D-313], che mette l'evidenza ACCANTO alla traccia proprio per
 * non far dipendere il formato dell'una da quello dell'altra.
 */
enum class ERTAuditFormatVersion : uint16
{
	/** Due conoscenze per squadra (Planning e Blast) piu' i verdetti delle voci del turno. */
	Initial = 1,

	/** Versione che questo binario SCRIVE. Chi aggiunge un campo alza questo alias e lascia il valore sopra. */
	Current = Initial
};

/**
 * Un verdetto **con il soggetto contro cui e' stato congelato**.
 *
 * 🔴 **Il soggetto non e' un extra: senza, il verdetto non e' verificabile.** `FreezeVerdictFor` lo congela
 * contro `StableUnitId`, `TeamId` e la cella dell'unita' **al momento della scrittura**, e nessuno dei tre
 * si ricava dalla voce archiviata — `SrcCell` e' la cella di PARTENZA del turno, il `TeamId` non c'e', e
 * `UnitId` su una voce di danno e' spesso chi SUBISCE. Ricalcolare da li' produrrebbe falsi
 * disallineamenti, cioe' un controllo che accusa il gioco di un difetto che non ha.
 *
 * ⚠️ **`HeroId`, `HeroDisplayName` e `bAlive` restano fuori** perche' `ClassifyTarget` non li legge: i
 * primi due sono dato di presentazione — una `FText` in un artefatto d'audit non ha niente da provare — e
 * il terzo non entra nella classificazione.
 */
USTRUCT()
struct FRTAuditVerdictRecord
{
	GENERATED_BODY()

	/**
	 * La fase in cui la voce e' nata, e **decide contro quale istantanea il verdetto va ricalcolato**.
	 *
	 * 🔴 **Trovato dal test d'integrazione su una partita vera, non previsto a tavolino.** La prima stesura
	 * ricalcolava tutto contro la conoscenza di Blast, e al turno 4 una voce divergeva: verdetto registrato
	 * `0x03`, ricalcolato `0x01`. Non era un difetto del gioco — era il controllo a guardare l'istantanea
	 * sbagliata. `TeamKnowledgeState` cambia **a meta' turno**, e una voce nata nel Dash porta il verdetto
	 * della conoscenza di **Planning**, non di quella del Blast.
	 *
	 * ⚠️ La regola: fasi **prima** di `Blast` -> istantanea di Planning; da `Blast` in poi -> quella di Blast.
	 * Sta qui e non si deduce dall'ordine di scrittura, perche' `SortTurnLog` riordina.
	 */
	UPROPERTY() ERTMatchPhase Phase = ERTMatchPhase::Planning;

	UPROPERTY() int32 SubjectUnitId = INDEX_NONE;
	UPROPERTY() int32 SubjectTeamId = 0;
	UPROPERTY() FRTCellId SubjectCell;
	UPROPERTY() FRTKnowledgeVerdict Verdict;
};

/**
 * L'evidenza d'audit di **un turno**, decisa da
 * [D-313](../../../docs/decisions/RT_PDR_00_Decision_Log.md).
 *
 * 🔴 **Vive accanto alla traccia e non dentro.** Un campo in piu' nel TurnLog costerebbe versione del
 * formato, `EntryLess`, `MixEntryFields` e gli undici golden; questo costa un formato nuovo e basta. E
 * l'obiezione della nota su `FRTTurnLogEntry::Verdict` — *«un verdetto e' una risposta alla presentazione,
 * non un fatto della simulazione»* — non si applica, perche' il replay pubblico questo file non lo legge.
 *
 * ⚠️ **Tre record, perche' le domande d'audit sono due e vogliono istanti diversi**: la conoscenza al
 * **Planning** e' cio' su cui il bot ha deciso, quella al **Blast** e' cio' contro cui i verdetti sono stati
 * congelati, e i verdetti sono il collegamento fra le due.
 */
USTRUCT()
struct FRTTurnAudit
{
	GENERATED_BODY()

	/** La partita a cui appartiene. Parte dell'aggancio: un audit di un'altra partita si riconosce. */
	UPROPERTY() FGuid MatchId;

	/** Il turno. L'altra meta' dell'aggancio: il **contenuto** vince sul nome del file. */
	UPROPERTY() int32 TurnNumber = 0;

	/**
	 * L'hash ordinato della traccia di quel turno, come il manifest lo registra.
	 *
	 * ⚠️ **E' l'aggancio vero, e non e' ridondante con i due campi sopra**: quelli dicono *a quale turno di
	 * quale partita* l'evidenza dice di appartenere, questo dice *a quale traccia*. Senza, un audit e una
	 * traccia rigenerata dello stesso turno sembrerebbero ancora una coppia.
	 */
	UPROPERTY() int64 OrderedHash = 0;

	/** La conoscenza per squadra al **Planning**: gli ingressi su cui il bot ha deciso. */
	UPROPERTY() TArray<FRTTeamKnowledge> PlanningKnowledge;

	/** La conoscenza per squadra al **Blast**: quella contro cui i verdetti sono congelati. */
	UPROPERTY() TArray<FRTTeamKnowledge> BlastKnowledge;

	/**
	 * I verdetti delle voci del turno, **in ordine canonico**.
	 *
	 * 🔴 **Indicizzati per POSIZIONE, e vale solo perche' la traccia archiviata e' gia' canonica**:
	 * `SortTurnLog` gira dentro la risoluzione e il recorder dichiara di non riordinare. E' esattamente la
	 * ragione per cui in memoria il campo vive **sull'entry** — *«un indice non sopravviverebbe al sort, il
	 * campo si'»* — e nell'archivio il vincolo cade. ⚠️ Un lettore che riordinasse attribuirebbe verdetti
	 * alla voce sbagliata **senza che niente lo segnali**.
	 */
	UPROPERTY() TArray<FRTAuditVerdictRecord> Verdicts;
};

/**
 * Scrive, rilegge e **interroga** l'evidenza d'audit.
 *
 * ⚠️ **Le due interrogazioni non sono un extra**: un archivio che *contiene* la conoscenza non dimostra
 * niente finche' qualcuno non rifa' il conto e lo confronta con cio' che il gioco aveva scritto. Sono loro
 * a rendere falsificabile l'AC di [D-276], invece che retorica.
 */
UCLASS()
class REFACTORTACTICS_API URTReplayAuditLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** `turn-001.rtaudit`, accanto a `turn-001.rtlog` e con lo stesso zero-padding. */
	static FString TurnAuditFileName(int32 TurnNumber);

	/** L'artefatto come JSON. Versione nel primo campo, come `ManifestToJson`. */
	static FString AuditToJson(const FRTTurnAudit& Audit);

	/** Rilegge. `false` = non leggibile, e `OutAudit` resta com'era. Fail-closed sulle versioni ignote. */
	static bool AuditFromJson(const FString& Json, FRTTurnAudit& OutAudit);

	/** Scrive l'artefatto del turno dentro la cartella della partita. */
	static bool RecordTurnAudit(const FString& ReplaysRoot, const FRTTurnAudit& Audit);

	/**
	 * Rilegge l'artefatto di un turno, **verificando l'aggancio**: se il contenuto dichiara un'altra partita
	 * o un altro turno, e' rifiutato. Il nome del file non e' una prova.
	 *
	 * Con `ExpectedOrderedHash` diverso da zero si verifica anche **l'ancora**: un artefatto che dichiara
	 * un'altra traccia dello stesso turno viene rifiutato. A zero significa «non lo so», e non si giudica.
	 */
	static bool LoadTurnAudit(const FString& ReplaysRoot, const FGuid& MatchId, int32 TurnNumber,
		FRTTurnAudit& OutAudit, int64 ExpectedOrderedHash = 0);

	/**
	 * Il verdetto che la conoscenza registrata produrrebbe per quel soggetto.
	 *
	 * 🔴 **Quale istantanea, lo decide `Record.Phase`**: prima del `Blast` quella di Planning, dal `Blast` in
	 * poi quella di Blast. `TeamKnowledgeState` ha due assegnazioni per turno, e una voce del Dash porta il
	 * verdetto della prima — lo ha insegnato un rosso su una partita vera, non un ragionamento.
	 *
	 * ⚠️ **Chiama `URTTeamKnowledgeLibrary::FreezeVerdict`, cioe' il predicato di PRODUZIONE.** Una copia
	 * scritta qui confronterebbe la copia con l'originale invece di confrontare il registrato col
	 * ricalcolato, e le due derive si coprirebbero a vicenda.
	 */
	static FRTKnowledgeVerdict RecomputeVerdict(const FRTTurnAudit& Audit, const FRTAuditVerdictRecord& Record);

	/**
	 * I verdetti **registrati** che non coincidono con quelli **ricalcolati**: e' l'anti-vacuita' di
	 * [D-223], che nessuno aveva mai misurato.
	 *
	 * 🔑 **Gli basta l'artefatto**: niente voci, niente tabelle passate da fuori. La coerenza dei verdetti si
	 * verifica leggendo **un file solo**, che e' cio' che un'evidenza d'audit dovrebbe permettere.
	 */
	static TArray<FString> FindVerdictMismatches(const FRTTurnAudit& Audit);

	// ⛔ **Il controllo d'EQUITA' non c'e', e l'assenza e' un risultato misurato.** Una prima stesura
	// confrontava la cella colpita con la conoscenza della squadra, ed e' insostenibile per due ragioni
	// indipendenti che una code review ha trovato:
	//
	//   · `TgtCell` su una voce di combattimento e' la cella della VITTIMA (`E.TgtCell = Units[Idx]->Cell`),
	//     non quella a cui il bot ha mirato. Un'area sparata verso una cella nota che investe un nemico
	//     ignoto in una cella adiacente e' **legittima**, e verrebbe segnalata come violazione;
	//   · il cancello di produzione che autorizza il bersaglio legge la conoscenza del **Blast**
	//     (`RTTurnManager_Blast.cpp`), non quella di Planning: un nemico entrato in vista nel Dash e'
	//     `Rejected` alla pianificazione e `Allowed` al Blast, e sparargli e' lecito.
	//
	// 🔴 **E sotto c'e' un problema piu' profondo del come**: la SCELTA del bot non e' archiviata, solo i suoi
	// EFFETTI. Verificare che abbia mirato a cio' che conosceva richiede il bersaglio scelto, che vorrebbe un
	// quarto record e quindi un emendamento a [D-313]. E' la stessa forma del limite gia' dichiarato
	// sull'armamento della reazione. Vedi `#2074`.
};
