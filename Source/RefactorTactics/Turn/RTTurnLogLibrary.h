#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: usato dalla firma di DescribeInvalidReason
#include "Turn/RTTurnLog.h"
#include "RTTurnLogLibrary.generated.h"

/**
 * Esito del confronto fra due tracce serializzate. I casi di CONTESTO (formato, topologia) sono distinti
 * dalla divergenza vera: due tracce prodotte con formati diversi non sono un bug del codice, e chiamarle
 * "divergenza" manderebbe a cercare un difetto dove c'e' una configurazione diversa.
 * Non e' UENUM: non serve ai Blueprint, e' un verdetto di libreria.
 */
enum class ERTTraceComparison : uint8
{
	/** Stesso formato, stessa topologia, stesse voci. */
	Identical,
	/** Le tracce dichiarano formati di partita diversi: non sono confrontabili. */
	FormatMismatch,
	/** Le tracce dichiarano topologie diverse: le celle non significano la stessa cosa. */
	TopologyMismatch,
	/** Stesso contesto, voci diverse: **questa** e' una divergenza. */
	Divergence,
	/** Almeno una delle due non e' una traccia valida (magic/versione/troncamento/checksum). */
	Unreadable
};

/**
 * Ordinamento e hash del TurnLog per l'osservabilita'/replay. Pura, deterministica, SOLO interi
 * (invariante #4: niente float). L'hash e' usato per la verifica di replay ("replay divergence = 0"),
 * mai per la logica di gioco.
 */
/**
 * Una voce del TurnLog resa leggibile, col suo soggetto e col verdetto congelato che porta da [D-223].
 *
 * 🔴 **Sostituisce un `TPair<FString, int32>`, e non e' cosmesi.** Quel `TPair` riciclava l'identita' in un
 * `int32` proprio sul canale che genera la maggior parte delle righe del combat log: `#1499` poteva
 * tipizzare tutti i call site sparsi e lasciare qui, dove le righe nascono davvero, un intero che il
 * compilatore non guarda.
 */
USTRUCT()
struct FRTDescribedLine
{
	GENERATED_BODY()

	UPROPERTY()
	FString Text;

	/**
	 * CHI la riga nomina. `INDEX_NONE` = voce di mondo; la traduzione dallo `0` del TurnLog avviene una
	 * volta sola, qui.
	 *
	 * ⛔ **NON e' una fonte di autorita', e non deve tornare a esserlo** (`#1499`). Chi puo' leggere questa
	 * riga lo dice `Verdict`, congelato quando il fatto e' accaduto ([D-223]) — questo campo serve a
	 * diagnosi, aggregazione e test.
	 *
	 * 🔴 **La distinzione ha una storia, ed e' il motivo per cui la riga esiste.** Prima di [D-223] il
	 * soggetto ERA il filtro, e il suo sentinella `INDEX_NONE` significava «riga senza soggetto» = **la
	 * leggono tutti**: un fail-open per omissione, cioe' il difetto che `#1499` ha aperto. Oggi il default
	 * di `Verdict` e' l'opposto — una riga senza verdetto **non si legge** — e riattaccare una decisione di
	 * privacy a questo `int32` la riporterebbe indietro **senza che il compilatore possa accorgersene**.
	 * Resta un `int32` per decisione: tipizzarlo proteggerebbe un campo che non decide nulla.
	 */
	UPROPERTY()
	int32 SubjectStableUnitId = INDEX_NONE;

	/** Il verdetto che la voce portava: si trasporta, non si ricalcola a valle. */
	UPROPERTY()
	FRTKnowledgeVerdict Verdict;
};

UCLASS()
class REFACTORTACTICS_API URTTurnLogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Ordine TOTALE deterministico fra due voci: Phase -> Category -> SrcCell(X,Y,Layer) ->
	 * TgtCell(X,Y,Layer) -> Outcome -> Amount -> ActionId -> TurnNumber -> GraphRevision -> UnitId.
	 * Vero se A precede B. Con un ordine totale, riordinare un insieme di voci da' sempre la stessa
	 * sequenza, indipendentemente dall'ordine d'inserimento.
	 *
	 * La catena copre OGNI campo che `SerializeTurnLog` scrive, e deve continuare a farlo: un campo scritto
	 * che il confronto non guarda lascia due voci a pari merito, e a decidere l'ordine resta un sort non
	 * stabile — cioe' due file diversi con lo stesso contenuto. Unica eccezione: `BaseActionId`, che e'
	 * funzione di `ActionId` e non puo' produrre pareggi.
	 *
	 * ⚠️ NON coincide con i campi dell'hash: `UnitId` e `TurnNumber` stanno qui e non in `MixEntryFields`.
	 * Chi vuole «uguali per l'hash» non usi questa funzione (vedi `GoldenEntriesMatch`).
	 */
	static bool EntryLess(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B);

	/**
	 * Due voci sono la STESSA voce per il confronto di un golden: uguali secondo l'hash, non secondo
	 * l'ordinamento.
	 *
	 * ⚠️ **Esposta il 2026-08-24 (CP 11.4, `#80`)**, e prima viveva in un namespace anonimo — mentre il
	 * commento di `EntryLess` qui sopra vi rimandava gia' per nome. Chi lo leggeva cercava una funzione che
	 * non poteva chiamare, e chi doveva sapere DOVE due tracce divergono finiva per riscriverne il
	 * criterio: due definizioni di «stessa voce» che nessun test confronta fra loro.
	 */
	static bool GoldenEntriesMatch(const FRTTurnLogEntry& A, const FRTTurnLogEntry& B);

	/**
	 * `UnitId` di questa voce porta CHI SUBISCE invece di chi ha agito?
	 *
	 * Le inversioni sono deliberate e documentate una per una in `FRTTurnLogEntry::UnitId`, ma finche' la
	 * risposta viveva solo in prosa un consumatore doveva **ricordarsi** di averla letta — e chi non l'ha
	 * letta ottiene un numero plausibile e sbagliato che nessun errore segnala. Stessa ragione per cui
	 * esiste `IsDamageInflictedByActor`: la tassonomia sta in un posto solo, e si CHIEDE.
	 *
	 * Copre il danno ambientale (`#625`, `#1067`) e le due voci di protezione scavalcata —
	 * `Facing`/`RearHitBypassedCover` e `Facing`/`RearHitBypassedGuard` (`#1418`, separate da `#1430`).
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|TurnLog")
	static bool IsSubjectTheSufferer(const FRTTurnLogEntry& Entry);

	/**
	 * Se questa causa fa NASCERE uno stato (`true`) o lo fa MORIRE (`false`).
	 *
	 * 🔴 **Esiste perché il verso non si deduca a occhio da nove valori.** Nascite: `AppliedByAction`,
	 * `AppliedByTerrain`, `AppliedWhileOnCell`, `AppliedInstantly`. Morti: `Revoked`, `Expired`,
	 * `Extinguished`, `Cleansed`, `Spent`. Chi consuma `ERTResolvedEventType::StatusChanged` chiede qui
	 * invece di scriversi il proprio `switch` — che sarebbe una seconda tassonomia da tenere allineata.
	 *
	 * ⚠️ **Un valore non dichiarato è una MORTE, e fail-closed è la scelta giusta qui**: un'icona che non
	 * si apre è un difetto visibile e correggibile; una che non si chiude resta accesa per sempre. Ma il
	 * ripiego non è la difesa — la difesa è `UndeclaredStatusOutcomes`, che rende rosso il caso.
	 */
	static bool IsStatusBirth(ERTStatusOutcome Outcome);

	/**
	 * I valori di `ERTStatusOutcome` che `IsStatusBirth` non dichiara, come nomi leggibili.
	 * **Vuoto significa copertura completa.**
	 *
	 * 🔑 **Itera l'enum VERO** (`StaticEnum<ERTStatusOutcome>()`), non una lista scritta a mano: un decimo
	 * valore aggiunto domani è coperto **per costruzione**. È la stessa disciplina di
	 * `URTPresentationBindingLibrary::FindMissingBindings` e `URTIconLibrary::FindMissingRequiredIcons`,
	 * con la stessa ragione — un contratto rotto si scopre in un test e non a schermo.
	 *
	 * ⚠️ Deterministica: l'ordine dell'uscita segue i valori dell'enum, mai quello di un `TMap`.
	 */
	static TArray<FString> UndeclaredStatusOutcomes();

	/**
	 * L'enum degli esiti che vale per una categoria, o `nullptr` se quella categoria non lo dichiara.
	 *
	 * `FRTTurnLogEntry::Outcome` e' un `uint8` il cui significato lo decide la CATEGORIA: `2` puo' essere
	 * `Lethal`, `AppliedWhileOnCell` o `BlockedByUnit`. Questa e' quella corrispondenza resa eseguibile, ed
	 * e' una proprieta' del TurnLog — non di chi lo legge.
	 *
	 * ⚠️ Stava in `URTScenarioLoader` fino al 2026-08-27 (`#1427`), e da li' non era raggiungibile: lo
	 * `ScenarioHarness` dipende da `Turn`, non viceversa, quindi il report della divergenza golden non
	 * poteva chiamarla e rendeva `Outcome` come intero nudo — mandando chi legge a cercare in `RTTurnLog.h`
	 * quale enum intendesse. Ricopiarne lo switch avrebbe creato un secondo elenco delle dieci categorie,
	 * cioe' la classe di difetto che `#1423` ha appena chiuso.
	 *
	 * ⚠️ Una categoria nuova senza il suo caso torna `nullptr`: chi la nomina riceve «esito sconosciuto»
	 * invece di un confronto fra interi che passerebbe per caso.
	 */
	static const UEnum* OutcomeEnumForCategory(ERTLogCategory Category);

	/** Il nome dell'esito per la sua categoria, o il numero grezzo se l'enum non lo conosce. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|TurnLog")
	static FString DescribeOutcome(ERTLogCategory Category, uint8 Outcome);

	/** Ordina il TurnLog in place con EntryLess (ordine totale deterministico). */
	static void SortTurnLog(TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Descrizione leggibile di una voce, con le celle in coordinate ASSIALI `(q=..,r=..,L=..)` e il reason
	 * code tradotto in italiano. Serve al combat log: senza, l'esito di un turno resta leggibile solo nel
	 * TurnLog binario e il giocatore non sa PERCHE' l'unita' non si e' mossa o il colpo non e' partito.
	 * Pura (nessun Actor, nessuno stato): il chiamante decide dove mostrarla.
	 */
	static FString DescribeEntry(const FRTTurnLogEntry& Entry);

	/**
	 * Testo italiano di un motivo di invalidita'. UNA tabella sola: `DescribeEntry` la usa per le voci di
	 * Fallback e il combat log la usa per il rifiuto al lock-in. Due tabelle divergerebbero al primo motivo
	 * aggiunto — ed e' successo: la prima stesura del lock-in stampava l'identificatore C++ dell'enum.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|TurnLog")
	static FString DescribeInvalidReason(ERTActionInvalidReason Reason);

	/**
	 * Prefisso dell'`ActionId` che dichiara una **causa di terreno** (`Terrain.Fire`, `Terrain.Ice`, ...).
	 *
	 * Vive qui e non nel `TurnManager` perche' lo condividono chi SCRIVE la voce e chi la INTERROGA: due
	 * letterali uguali per abitudine sono la seconda verita' che `D-098` vieta, e diverge il giorno in cui
	 * uno dei due cambia.
	 */
	static const TCHAR* TerrainCausePrefix() { return TEXT("Terrain."); }

	/**
	 * La voce e' **danno AMBIENTALE**, cioe' una di quelle in cui `UnitId` porta chi SUBISCE (`#1150`).
	 *
	 * Sono due, e la loro causa sta nell'`ActionId`: `Terrain.<Surface>` all'ingresso (`#1067`) e
	 * `Status.Burning` nel Cleanup (`#625`). La seconda si CHIEDE a `TAG_Status_Burning`, non si riscrive.
	 *
	 * ⚠️ **L'elenco delle cause fallirebbe APERTO, e per questo non e' solo un elenco.** Una causa nuova —
	 * `#1077` sta portando gli stati nel TurnLog — che nessuno aggiungesse qui verrebbe classificata come
	 * danno INFLITTO, cioe' accreditata a chi la subisce: il verso pericoloso. La rete e' la forma della
	 * voce, `SrcCell == TgtCell`, che un attacco non puo' avere. ⛔ Resta **secondaria** di proposito:
	 * `AppendLogEntry` dichiara che `SrcCell` non identifica l'unita', e farne la regola sarebbe l'inferenza
	 * che il formato ha smesso di sostenere quando `UnitId` e' nato (`D-063`).
	 *
	 * ⚠️ Falso positivo noto, e la sua direzione: un'area con fuoco amico che investa la cella di chi la
	 * lancia viene contata come «subita». Sottostima il danno inflitto invece di gonfiarlo.
	 */
	static bool IsEnvironmentalDamage(const FRTTurnLogEntry& Entry);

	/**
	 * La voce e' **danno che `UnitId` ha inflitto a qualcun altro**: la domanda di chi aggrega il danno per
	 * unita', e la ragione per cui `#1150` esiste.
	 *
	 * 🔴 **Il danno inflitto NON vive solo in `Combat`.** Overwatch lo scrive come `ReactionDecision`
	 * (`FireChosen`, attore `WatchOwner`) e la previsione come `Predictive` (`TriggerMatched`, attore
	 * `Shooter`). Filtrare la sola `Combat` restituiva **zero** per un `InterceptShot` andato a segno — lo
	 * stesso numero plausibile e sbagliato che questa API esiste per impedire, nel verso opposto.
	 *
	 * ⚠️ **L'esito si legge per categoria, e `Amount` da solo non basta**: in `Fallback` quel campo porta un
	 * `ERTActionInvalidReason`, non un danno. Un predicato «`Amount > 0`» sommerebbe codici di errore.
	 *
	 * ⚠️ **`UnitId == 0` e' falso**, sempre: lo zero significa «nessuna unita' dichiarata», e un predicato
	 * che si chiama «inflitto da un attore» non puo' essere vero dove l'attore non c'e'.
	 *
	 * ⚠️ **`Healed` e `NoLineOfSight` restano fuori** e non e' ovvio in nessuno dei due: la cura ha un agente
	 * vero — `UnitId` e' chi cura — ma non e' danno; un attacco fermato dalla copertura ha agente e categoria
	 * giusti, e zero danno inflitto. Contarli darebbe due numeri sbagliati in versi opposti.
	 *
	 * ⛔ **I due predicati NON partizionano il TurnLog**: si escludono ma non esauriscono. `Healed`,
	 * `NoLineOfSight` e ogni categoria non di danno non soddisfano nessuno dei due, e un consumatore che
	 * sottraesse l'uno dall'altro contando su una partizione otterrebbe un residuo che non e' danno subito.
	 */
	static bool IsDamageInflictedByActor(const FRTTurnLogEntry& Entry);


	/**
	 * Il TurnLog intero in forma leggibile: una riga per voce, nell'ordine CANONICO.
	 *
	 * E' la meta' che mancava a `DescribeEntry`, ed e' la ragione per cui #79 chiede un log «coerente col
	 * TurnLog serializzato: stesse informazioni, stesso ordine». Finche' le righe leggibili nascono sparse
	 * durante la risoluzione e il TurnLog nasce altrove, i due sono due produttori indipendenti: coincidono
	 * per abitudine, non per costruzione, e nessuno se ne accorge il giorno in cui smettono.
	 *
	 * Ordina con `SortTurnLog` prima di descrivere, cosi' la sequenza non dipende dall'ordine di arrivo.
	 * Pura: nessun Actor, nessuno stato.
	 *
	 * ⚠️ Le righe escono SENZA soggetto. Chi le mostra a un giocatore — cioe' chi deve poterle filtrare per
	 * conoscenza — usi `DescribeTurnLogWithSubjects`.
	 */
	static TArray<FString> DescribeTurnLog(TArray<FRTTurnLogEntry> Entries);

	/**
	 * Le STESSE righe di `DescribeTurnLog`, ciascuna col SOGGETTO che l'ha prodotta.
	 *
	 * 🔴 Esiste perche' il combat log del giocatore si **deriva** da qui (CP 11.3, `#79`): senza soggetto
	 * ogni riga derivata arriva a `ARTTurnManager::AddLogEvent` come «riga di mondo» e passa il filtro di
	 * conoscenza SEMPRE — comprese quelle che stampano `SrcCell`/`TgtCell` di un nemico che la squadra non
	 * vede. Il canale primario non puo' essere l'unico che non porta l'informazione per filtrarsi.
	 *
	 * Il soggetto e' `FRTTurnLogEntry::UnitId`, che e' uno `StableUnitId` ([D-063]). ⚠️ **`UnitId == 0`
	 * diventa `INDEX_NONE`**: lo zero significa «nessuna unita' dichiarata» e gli `StableUnitId` partono da
	 * 1, mentre a valle il sentinella di «riga senza soggetto» e' `INDEX_NONE`. Due sentinelle diverse per
	 * la stessa assenza: la traduzione sta qui, in un posto solo.
	 *
	 * ⚠️ Per il danno AMBIENTALE `UnitId` porta chi SUBISCE, non chi ha agito (`#1150`). Per questa domanda
	 * — «chi deve conoscere l'unita' per leggere la riga?» — e' il soggetto giusto: la riga nomina la
	 * vittima e la sua cella.
	 *
	 * `DescribeTurnLog` e' un adattatore su questa: un solo produttore, quindi testo e ordine non possono
	 * divergere fra le due forme.
	 *
	 * 🔴 **Le righe di MOVIMENTO nominano il soggetto anche nel TESTO** (`#1932`). Fino ad allora il soggetto
	 * viaggiava solo in `SubjectStableUnitId` — serviva al filtro di conoscenza, e chi LEGGEVA la riga non
	 * l'aveva. Costo misurato: [#1733](../../../docs/roadmap/plans/unita-sovrapposte-1733-spec-panel-2026-08-30.md)
	 * e' stata aperta come bug di gameplay perche' due righe della **stessa** unita' — *«si muove (-1,-1) ->
	 * (1,-1)»* nel Dash e *«resta (1,-1)»* nel Move — si leggevano come due unita' sulla stessa cella. Una
	 * sovrapposizione che non e' mai avvenuta, e una revisione con panel per smontarla.
	 *
	 * ⚠️ **Solo `ERTLogCategory::Move`, e non e' timidezza**: il prefisso funziona dove il soggetto e' anche
	 * il soggetto GRAMMATICALE del predicato. Per il danno `UnitId` porta chi **subisce** (`#1150`), e
	 * *«Gadget: colpisce»* direbbe il falso; le voci `Status` cominciano gia' con la cella. Estendere ad
	 * altre categorie vuole prima un predicato che regga il soggetto davanti.
	 *
	 * @param SubjectNames  `StableUnitId` -> nome leggibile (`ARTUnit::DisplayLabel`). Chi manca ricade su
	 *                      `u<id>`, che e' verificabile e non mente; una mappa vuota e' legittima e da' righe
	 *                      con i soli id. La libreria resta **pura**: i nomi li risolve il chiamante, che ha
	 *                      gli Actor.
	 */
	static TArray<FRTDescribedLine> DescribeTurnLogWithSubjects(TArray<FRTTurnLogEntry> Entries,
		const TMap<int32, FString>& SubjectNames = TMap<int32, FString>());

	/**
	 * L'identita' dell'azione di una voce, come **azione base + profilo** quando la voce sa dirlo:
	 * `Action.BasicAttack · Riktor.ImpactShot`. E' la forma che [D-033](../../../docs/decisions/RT_PDR_00_Decision_Log.md)
	 * chiede — «spiegabile nel TurnLog come azione base + profilo» — e senza di essa una traccia dice solo
	 * `Riktor.ImpactShot`, che e' un'azione d'EROE e non si risolve col catalogo core.
	 *
	 * Ricade sul solo `ActionId` quando `BaseActionId` e' vuoto (traccia di formato < 5, o azione che non e'
	 * profilo di niente) o quando i due coincidono. Pura: legge solo la voce.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|TurnLog")
	static FString DescribeActionIdentity(const FRTTurnLogEntry& Entry);

	/**
	 * Hash intero (FNV-1a sui campi interi) del TurnLog, PERMUTAZIONE-INVARIANTE (ordina con EntryLess
	 * prima di mescolare). Deterministico, solo interi. Uso: verifica di replay, mai logica di gioco.
	 */
	static uint32 HashTurnLog(const TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Hash intero del TurnLog SENSIBILE ALL'ORDINE DI APPEND: mescola le voci come sono arrivate, senza
	 * ordinarle. Stessi campi di `HashTurnLog`, stesso FNV-1a: cambia solo che non c'e' il sort davanti.
	 *
	 * Esiste perche' `HashTurnLog` e' cieco al riordino **per scelta**, e quella cecita' lascia scoperto un
	 * difetto reale: un ciclo che iterasse una `TMap` cambierebbe la sequenza emessa dal resolver senza
	 * cambiare un solo hash, e nessun corpus golden se ne accorgerebbe. I due hash rispondono a domande
	 * diverse e servono entrambi ([D-062](../../../docs/decisions/RT_PDR_00_Decision_Log.md)).
	 *
	 * ⚠️ **Non e' ricalcolabile da un file.** `SerializeTurnLog` scrive in forma canonica (ordinata), quindi
	 * i byte perdono l'ordine di emissione: questo hash si calcola sulla traccia IN MEMORIA, prima di
	 * serializzare, e chi lo vuole conservare lo mette nel proprio header — non in quello del TurnLog, che
	 * deve restare permutazione-invariante (`D-SR-1`, e il test `SerializeCanonicalPermutationInvariant`).
	 */
	static uint32 HashTurnLogOrdered(const TArray<FRTTurnLogEntry>& Entries);

	/**
	 * Serializza il TurnLog in un buffer binario VERSIONATO e in forma CANONICA (ordina con SortTurnLog
	 * prima di scrivere -> byte permutazione-invarianti, come l'hash). Header: magic + versione +
	 * topologia (flags) + conteggio; poi ogni voce come interi little-endian espliciti (invariante #4).
	 * Topology dichiara come vanno lette le celle (offset quadrate o assiali esagonali); il default Square
	 * scrive flags = 0, quindi i byte restano identici a quelli prodotti prima di questa estensione.
	 */
	static TArray<uint8> SerializeTurnLog(const TArray<FRTTurnLogEntry>& Entries,
		ERTLogTopology Topology = ERTLogTopology::Square, FName FormatId = NAME_None);

	/**
	 * Ricostruisce il TurnLog da un buffer prodotto da SerializeTurnLog. Ritorna false (fail-closed, nessun
	 * crash) se magic/versione/topologia non riconosciuti o buffer troncato; in tal caso OutEntries e' svuotato.
	 * Se OutTopology != nullptr riceve la topologia dichiarata nel file; se OutFormatId != nullptr riceve
	 * l'identita' del formato (`NAME_None` per le tracce scritte prima della versione 4).
	 * Round-trip garantito a livello di hash: HashTurnLog(in) == HashTurnLog(out).
	 */
	static bool DeserializeTurnLog(const TArray<uint8>& Bytes, TArray<FRTTurnLogEntry>& OutEntries,
		ERTLogTopology* OutTopology = nullptr, FName* OutFormatId = nullptr);

	/**
	 * Salva il TurnLog su file (serializzazione binaria versionata + checksum). Ritorna false se la
	 * scrittura fallisce. Thin wrapper su SerializeTurnLog + FFileHelper.
	 */
	static bool SaveTurnLogToFile(const FString& Path, const TArray<FRTTurnLogEntry>& Entries,
		ERTLogTopology Topology = ERTLogTopology::Square, FName FormatId = NAME_None);

	/**
	 * Carica il TurnLog da file. Ritorna false (fail-closed, OutEntries svuotato) se il file manca o il
	 * contenuto e' invalido/corrotto (magic/versione/topologia/troncamento/checksum). Se OutTopology != nullptr
	 * riceve la topologia dichiarata nel file; se OutFormatId != nullptr l'identita' del formato.
	 * Wrapper su FFileHelper + DeserializeTurnLog.
	 */
	static bool LoadTurnLogFromFile(const FString& Path, TArray<FRTTurnLogEntry>& OutEntries,
		ERTLogTopology* OutTopology = nullptr, FName* OutFormatId = nullptr);

	/**
	 * Confronta due tracce serializzate verificando il CONTESTO prima del contenuto: formato, poi topologia,
	 * poi le voci (per hash). L'hash non copre formato e topologia — per non invalidare gli hash golden gia'
	 * registrati — quindi deve coprirli questa procedura, o il falso "nessuna divergenza" ricompare un piano
	 * piu' su (issue #185, spec §16.3).
	 */
	static ERTTraceComparison CompareSerializedTraces(const TArray<uint8>& A, const TArray<uint8>& B);

	/**
	 * Descrive la PRIMA divergenza fra una traccia di riferimento e quella appena prodotta, oppure una stringa
	 * vuota se coincidono (CP 12.6, #178).
	 *
	 * `CompareSerializedTraces` risponde al livello del formato — *sono diverse* — e per un corpus golden non
	 * basta: il DoD chiede che una divergenza indichi **turno, fase e `ActionId`**, perche' un test che dice
	 * solo «hash diverso» costringe chi legge a ricostruire da capo *dove*, ed e' cosi' che un corpus finisce
	 * rigenerato invece che letto.
	 *
	 * Il TURNO viene passato dal chiamante e non letto da `FRTTurnLogEntry::TurnNumber` (che dal formato v6
	 * esiste, ma nessun produttore lo valorizza e le tracce < v6 lo portano a `0`): il chiamante lo sa, che
	 * lo conosce: nel corpus e' il file da cui la traccia di riferimento e' stata letta.
	 *
	 * Riporta la PRIMA differenza e non tutte: dopo la prima, le successive sono spesso conseguenze.
	 */
	static FString DescribeFirstDivergence(int32 TurnNumber, const TArray<FRTTurnLogEntry>& Golden,
		const TArray<FRTTurnLogEntry>& Actual);
};
