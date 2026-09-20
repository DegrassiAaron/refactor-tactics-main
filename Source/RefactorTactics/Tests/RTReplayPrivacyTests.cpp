#include "Misc/AutomationTest.h"
#include "Replay/RTReplayPrivacyLibrary.h"
#include "Replay/RTReplayPlayerLibrary.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTReplayViewerSubsystem.h"
#include "Tests/RTReplayTestFixtures.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il confine fra i DUE prodotti di [D-276]: il Replay Pubblico/Sanitizzato e la Traccia di Audit Privata.
 *
 * ⚠️ **Questi test non guardano dei valori, guardano dei CAMPI.** E' la differenza fra un gate che dimostra
 * che oggi non c'e' un leak e uno che dimostra che domani non ci puo' essere: il primo si scrive con un
 * assert su una voce, e resta verde quando qualcuno aggiunge il diciannovesimo campo.
 */
namespace
{
	/** I nomi delle `UPROPERTY` di una struct riflessa. */
	TSet<FName> ReflectedNames(const UStruct* Type)
	{
		TSet<FName> Out;
		for (TFieldIterator<FProperty> It(Type); It; ++It)
		{
			Out.Add(It->GetFName());
		}
		return Out;
	}

	FString Listed(const TSet<FName>& Names)
	{
		TArray<FString> As;
		for (const FName& N : Names) { As.Add(N.ToString()); }
		As.Sort();
		return FString::Join(As, TEXT(", "));
	}

	/**
	 * Una voce con un valore DIVERSO da quello di default in **ogni** campo, audit compresi.
	 *
	 * ⚠️ `Verdict` incluso, e non e' un dettaglio: e' il campo che la issue chiama *«l'unico che porta la
	 * conoscenza»*, e lasciarlo al default renderebbe vacuo qualunque test futuro che volesse dimostrare
	 * che non raggiunge il prodotto pubblico — confronterebbe un default con un default.
	 */
	FRTTurnLogEntry SaturatedEntry()
	{
		FRTTurnLogEntry E;
		E.Phase = ERTMatchPhase::Blast;
		E.Category = ERTLogCategory::Reaction;
		E.Outcome = 3;
		E.SrcCell = FRTCellId(1, 2, 0);
		E.TgtCell = FRTCellId(3, 4, 1);
		E.Amount = 17;
		E.ActionId = FName(TEXT("Hero.Aevik.ArcPulse"));
		E.BaseActionId = FName(TEXT("Action.BasicAttack"));
		E.UnitId = 42;
		E.Verdict = FRTKnowledgeVerdict::Everyone();
		E.TurnNumber = 7;
		E.GraphRevision = 9;
		E.Priority = 55;
		// `#1880`: la terza coordinata del boundary. Un valore diverso dal default, come ogni altro campo
		// qui — la voce e' satura proprio perche' un campo lasciato al default non distinguerebbe
		// «trasportato» da «dimenticato».
		E.MicroStepIndex = 6;
		E.OpportunityId = TEXT("OPP-1");
		E.ReactionInstanceId = 11;
		E.SelectedTargetUnitId = 43;
		E.OriginalTargetUnitId = 44;
		E.ReactionResponse = TEXT("FIRE");
		// `SightBlockerCell` (`#2534`, v13): il muro che ferma il tiro. Va **diverso dal default**
		// `NoSightBlocker()` — `Layer = INDEX_NONE` — o il campo resterebbe al proprio default e
		// `PublicFieldsKeepTheirValue` fallirebbe dichiarandolo «fermo», che e' esattamente il difetto che
		// quel test esiste per prendere: un campo pubblico che nessuno copia passa inosservato finche' la
		// fixture non lo satura.
		E.SightBlockerCell = FRTCellId(2, -1, 1);
		return E;
	}
}

/**
 * Il gate che l'AC di #1805 chiede per nome: *«un campo audit-only aggiunto al modello non deve poter
 * finire nell'export pubblico senza far diventare rosso un test»*.
 *
 * Non lo si ottiene guardando i valori. Lo si ottiene **obbligando a classificare**: chi aggiunge una
 * `UPROPERTY` a `FRTTurnLogEntry` e non dice se e' pubblica o di audit trova questo rosso, e la
 * classificazione diventa una decisione presa invece che un default subito.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacyClassificationTest,
	"RefactorTactics.Replay.Privacy.EveryLoggedFieldIsClassified",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacyClassificationTest::RunTest(const FString&)
{
	const TMap<FName, ERTReplayFieldVisibility>& Table = URTReplayPrivacyLibrary::FieldVisibility();
	const TSet<FName> Reflected = ReflectedNames(FRTTurnLogEntry::StaticStruct());

	// Anti-vacuita': una tabella vuota renderebbe verdi i due controlli sotto per assenza di soggetto.
	TestTrue(TEXT("la tabella classifica almeno un campo"), Table.Num() > 0);
	TestTrue(TEXT("la reflection vede almeno un campo di FRTTurnLogEntry"), Reflected.Num() > 0);

	TSet<FName> Unclassified;
	for (const FName& N : Reflected)
	{
		if (!Table.Contains(N)) { Unclassified.Add(N); }
	}
	TestTrue(
		FString::Printf(TEXT("ogni campo di FRTTurnLogEntry e' classificato; non classificati: [%s]"),
			*Listed(Unclassified)),
		Unclassified.Num() == 0);

	// Il difetto simmetrico: un campo rinominato lascerebbe nella tabella un nome che non esiste piu', e la
	// classificazione smetterebbe di riguardare qualcosa. `GET_MEMBER_NAME_CHECKED` lo previene in
	// compilazione; questo lo misura comunque, perche' un gate che dipende da una macro corretta a mano non
	// e' un gate.
	TSet<FName> Ghosts;
	for (const TPair<FName, ERTReplayFieldVisibility>& Row : Table)
	{
		if (!Reflected.Contains(Row.Key)) { Ghosts.Add(Row.Key); }
	}
	TestTrue(
		FString::Printf(TEXT("la tabella non classifica campi inesistenti; fantasmi: [%s]"), *Listed(Ghosts)),
		Ghosts.Num() == 0);

	// Una chiave duplicata verrebbe ingoiata dalla `TMap` con l'ultima riga vincente: il conteggio la vede.
	TestEqual(TEXT("una riga per campo, nessuna classificata due volte"), Table.Num(), Reflected.Num());

	return true;
}

/**
 * La biiezione fra la classificazione e il tipo pubblico, che chiude i due difetti opposti: un campo di
 * audit che scivola nel prodotto pubblico, e un campo classificato pubblico che non viene mai esportato —
 * il secondo non e' un leak, ma e' una classificazione che mente.
 *
 * ⚠️ **Confronta anche il TIPO, non solo il nome.** Cambiare `int32 Amount` in `float Amount` nel solo
 * tipo pubblico continuerebbe a compilare, a convertire implicitamente e a passare un confronto fra nomi:
 * il prodotto pubblico cambierebbe semantica senza che niente diventi rosso, contro l'invariante 14 di
 * `AGENTS.md` §3.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacyPublicTypeTest,
	"RefactorTactics.Replay.Privacy.PublicEntryMatchesTheClassification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacyPublicTypeTest::RunTest(const FString&)
{
	const TMap<FName, ERTReplayFieldVisibility>& Table = URTReplayPrivacyLibrary::FieldVisibility();
	const TSet<FName> InPublicType = ReflectedNames(FRTPublicReplayEntry::StaticStruct());

	TSet<FName> ClassifiedPublic;
	TSet<FName> ClassifiedAudit;
	for (const TPair<FName, ERTReplayFieldVisibility>& Row : Table)
	{
		(Row.Value == ERTReplayFieldVisibility::Public ? ClassifiedPublic : ClassifiedAudit).Add(Row.Key);
	}

	TestTrue(TEXT("almeno un campo e' classificato pubblico"), ClassifiedPublic.Num() > 0);
	TestTrue(TEXT("almeno un campo e' classificato audit-only"), ClassifiedAudit.Num() > 0);

	const TSet<FName> AuditInsidePublic = InPublicType.Intersect(ClassifiedAudit);
	TestTrue(
		FString::Printf(TEXT("nessun campo audit-only vive dentro FRTPublicReplayEntry; trovati: [%s]"),
			*Listed(AuditInsidePublic)),
		AuditInsidePublic.Num() == 0);

	const TSet<FName> MissingFromPublic = ClassifiedPublic.Difference(InPublicType);
	TestTrue(
		FString::Printf(TEXT("ogni campo classificato pubblico esiste nel tipo pubblico; mancanti: [%s]"),
			*Listed(MissingFromPublic)),
		MissingFromPublic.Num() == 0);

	const TSet<FName> UnclassifiedInPublic = InPublicType.Difference(ClassifiedPublic);
	TestTrue(
		FString::Printf(TEXT("il tipo pubblico non porta campi fuori dalla classificazione; di troppo: [%s]"),
			*Listed(UnclassifiedInPublic)),
		UnclassifiedInPublic.Num() == 0);

	TSet<FName> TypeMismatch;
	for (const FName& N : ClassifiedPublic)
	{
		const FProperty* Source = FRTTurnLogEntry::StaticStruct()->FindPropertyByName(N);
		const FProperty* Target = FRTPublicReplayEntry::StaticStruct()->FindPropertyByName(N);
		if (!Source || !Target || !Source->SameType(Target)) { TypeMismatch.Add(N); }
	}
	TestTrue(
		FString::Printf(TEXT("un campo pubblico ha lo stesso TIPO nei due prodotti; divergenti: [%s]"),
			*Listed(TypeMismatch)),
		TypeMismatch.Num() == 0);

	return true;
}

/**
 * Il ponte copia i campi pubblici **con il loro valore**.
 *
 * ⚠️ Il nome dice cio' che il corpo misura, e non di piu': la **caduta** dei campi di audit e' strutturale
 * — non c'e' un campo dove finire — e la misura `PublicEntryMatchesTheClassification`. Qui serve l'altra
 * meta', che si dimentica sempre: un sanitizer che azzerasse tutto passerebbe qualunque test di privacy e
 * non sarebbe un replay.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacyCopyTest,
	"RefactorTactics.Replay.Privacy.PublicFieldsKeepTheirValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacyCopyTest::RunTest(const FString&)
{
	const FRTTurnLogEntry Audit = SaturatedEntry();
	const TArray<FRTPublicReplayEntry> Public = URTReplayPrivacyLibrary::ToPublicTrace({ Audit });

	if (!TestEqual(TEXT("una voce di audit produce una voce pubblica"), Public.Num(), 1))
	{
		return false;
	}

	// 🔴 Il confronto passa dalla REFLECTION e non da un elenco scritto qui: un elenco a mano sarebbe la
	// terza lista degli stessi campi, e si fermerebbe ai campi di oggi. Cosi' cresce col tipo.
	const FRTPublicReplayEntry Default;
	TSet<FName> NotCopied;
	TSet<FName> LeftAtDefault;
	for (TFieldIterator<FProperty> It(FRTPublicReplayEntry::StaticStruct()); It; ++It)
	{
		const FProperty* Target = *It;
		const FProperty* Source = FRTTurnLogEntry::StaticStruct()->FindPropertyByName(Target->GetFName());
		if (!Source || !Source->SameType(Target))
		{
			NotCopied.Add(Target->GetFName());
			continue;
		}

		if (!Target->Identical(
			Target->ContainerPtrToValuePtr<void>(&Public[0]),
			Source->ContainerPtrToValuePtr<void>(&Audit)))
		{
			NotCopied.Add(Target->GetFName());
		}

		// Anti-vacuita' del confronto: se il valore saturo coincidesse col default, l'uguaglianza sopra
		// sarebbe vera anche per un campo mai copiato.
		if (Target->Identical(
			Target->ContainerPtrToValuePtr<void>(&Public[0]),
			Target->ContainerPtrToValuePtr<void>(&Default)))
		{
			LeftAtDefault.Add(Target->GetFName());
		}
	}

	TestTrue(
		FString::Printf(TEXT("ogni campo del tipo pubblico porta il valore della voce di audit; divergenti: [%s]"),
			*Listed(NotCopied)),
		NotCopied.Num() == 0);
	TestTrue(
		FString::Printf(TEXT("la voce satura non lascia nessun campo pubblico al proprio default; fermi: [%s]"),
			*Listed(LeftAtDefault)),
		LeftAtDefault.Num() == 0);

	return true;
}

/**
 * Determinismo: nessun riordino, nessuna sorgente di variazione fra due chiamate (`D-263`).
 *
 * L'ordine non e' un dettaglio di comodo: la traccia arriva gia' ordinata da `SortTurnLog`, e un sanitizer
 * che riordinasse produrrebbe un replay che non e' quello che la partita ha risolto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacyDeterminismTest,
	"RefactorTactics.Replay.Privacy.SanitizeIsOrderPreservingAndPure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacyDeterminismTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Audit;
	for (int32 i = 0; i < 8; ++i)
	{
		FRTTurnLogEntry E = SaturatedEntry();
		E.Amount = i;          // pubblico: distingue le voci nel prodotto
		E.UnitId = 100 + i;    // pubblico: una seconda chiave, perche' una sola non e' un ordine
		E.OpportunityId = FString::Printf(TEXT("OPP-%d"), 100 - i); // audit: non deve arrivare ne' ordinare
		Audit.Add(E);
	}

	const TArray<FRTPublicReplayEntry> First = URTReplayPrivacyLibrary::ToPublicTrace(Audit);
	const TArray<FRTPublicReplayEntry> Second = URTReplayPrivacyLibrary::ToPublicTrace(Audit);

	TestEqual(TEXT("il conteggio si conserva"), First.Num(), Audit.Num());

	bool bOrderPreserved = First.Num() == Audit.Num();
	for (int32 i = 0; bOrderPreserved && i < First.Num(); ++i)
	{
		bOrderPreserved = First[i].Amount == Audit[i].Amount && First[i].UnitId == Audit[i].UnitId;
	}
	TestTrue(TEXT("l'ordine di ingresso e' quello di uscita"), bOrderPreserved);

	// 🔴 Il confronto fra le due chiamate passa dalla struct INTERA e non da due campi scelti a mano: una
	// coppia di campi si ferma ai campi di oggi, e un'implementazione che rendesse non deterministico
	// `ActionId` o `GraphRevision` passerebbe lo stesso.
	bool bIdentical = First.Num() == Second.Num();
	for (int32 i = 0; bIdentical && i < First.Num(); ++i)
	{
		bIdentical = FRTPublicReplayEntry::StaticStruct()->CompareScriptStruct(&First[i], &Second[i], 0);
	}
	TestTrue(TEXT("due chiamate sullo stesso ingresso danno voci identiche in ogni campo"), bIdentical);

	return true;
}

/**
 * Il confine ha un consumatore, e non e' una libreria che nessuno chiama.
 *
 * 🔴 `URTReplayViewerSubsystem::GetCurrentPhaseEntries` e' `BlueprintCallable`: e' **la** superficie che un
 * widget puo' raggiungere, ed e' cio' che [D-276] §2 chiama *«la riproduzione spettatore»*. Finche'
 * consegnava `FRTTurnLogEntry`, ogni widget legato a quel nodo leggeva `Verdict`, `OpportunityId`,
 * `ReactionInstanceId` e `ReactionResponse` di **entrambe** le squadre.
 *
 * Il controllo e' di COMPILAZIONE perche' il difetto lo e': il tipo di ritorno non si degrada a runtime.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacySpectatorSurfaceTest,
	"RefactorTactics.Replay.Privacy.SpectatorSurfaceHandsOutPublicEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacySpectatorSurfaceTest::RunTest(const FString&)
{
	using FSpectatorReturn = decltype(DeclVal<const URTReplayViewerSubsystem>().GetCurrentPhaseEntries());
	static_assert(std::is_same_v<FSpectatorReturn, TArray<FRTPublicReplayEntry>>,
		"La superficie spettatore deve consegnare voci pubbliche: con FRTTurnLogEntry un widget legge i campi di audit");

	TestTrue(TEXT("la superficie spettatore consegna voci pubbliche (verificato in compilazione)"), true);
	return true;
}

// =====================================================================================================
// Il confine per le VOCI ([D-316], `#2098`)
// =====================================================================================================

namespace
{
	/** Una voce riconoscibile dal suo `Amount`, con un verdetto dichiarato. */
	FRTTurnLogEntry VoceConVerdetto(int32 Amount, const FRTKnowledgeVerdict& Verdetto)
	{
		FRTTurnLogEntry E;
		E.Phase = ERTMatchPhase::Blast;
		E.Category = ERTLogCategory::Combat;
		E.TurnNumber = 1;
		E.Amount = Amount;
		E.ActionId = FName(TEXT("Action.BasicAttack"));
		E.SrcCell = FRTCellId(1, 0);
		E.TgtCell = FRTCellId(2, 0);
		E.Verdict = Verdetto;
		return E;
	}

	FRTKnowledgeVerdict SoloSquadra(int32 TeamId)
	{
		FRTKnowledgeVerdict V;
		V.AllowTeam(TeamId);
		return V;
	}

	/** Gli `Amount` presenti, che qui fanno da identita' delle voci. */
	TArray<int32> Importi(const TArray<FRTTurnLogEntry>& Voci)
	{
		TArray<int32> Out;
		for (const FRTTurnLogEntry& E : Voci) { Out.Add(E.Amount); }
		return Out;
	}
}

/**
 * **Una voce che l'osservatore non conosceva non raggiunge la sua traccia** ([D-316], `#2098`).
 *
 * 🔴 **Il difetto che chiude: `ToPublicTrace` filtrava i CAMPI, non le VOCI.** Lo spettatore riceveva una
 * riga per ogni fatto del turno, comprese quelle di unita' che la sua squadra non aveva mai visto — con le
 * colonne di audit tolte, ma la riga c'era, e il solo fatto che ci fosse dice *«qualcuno ha fatto qualcosa
 * li'»*.
 *
 * ⚠️ **Il test gira sul percorso VERO — registra su disco e riapre — e non sul solo predicato**, ed e' una
 * scelta: il predicato da solo resterebbe verde anche se il recorder non scrivesse mai le tracce per
 * osservatore, o se il player non le aprisse. E' l'intera catena di [D-316] a dover reggere, e il punto
 * dove si romperebbe piu' facilmente e' la giuntura fra i due.
 *
 * ⛔ **ANTI-VACUITA': ogni squadra ha una voce che DEVE passare e una che NON deve.** Un filtro che
 * lasciasse passare tutto fallirebbe la seconda meta'; uno che bloccasse tutto — il modo in cui questo
 * meccanismo si rompe davvero, perche' `AllowsTeam` e' fail-closed su una maschera azzerata — fallirebbe la
 * prima. Nessuno dei due errori sopravvive.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacyObserverEntriesTest,
	"RefactorTactics.Replay.Privacy.ObserverTraceOmitsUnknownEntries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacyObserverEntriesTest::RunTest(const FString&)
{
	const FString Root = RTReplayFixtures::TransientRoot(TEXT("ObserverEntries"));
	RTReplayFixtures::Pulisci(Root);

	// Tre fatti nello stesso turno: uno che solo la squadra 0 conosceva, uno che solo la 1 conosceva, e uno
	// pubblico — un ponte che crolla, che `AppendLogEntry` congela a `Everyone()` quando non c'e' un attore.
	TArray<FRTTurnLogEntry> Voci;
	Voci.Add(VoceConVerdetto(10, SoloSquadra(0)));
	Voci.Add(VoceConVerdetto(20, SoloSquadra(1)));
	Voci.Add(VoceConVerdetto(30, FRTKnowledgeVerdict::Everyone()));
	URTTurnLogLibrary::SortTurnLog(Voci);

	FRTReplayManifest M;
	M.MatchId = FGuid::NewGuid();
	M.FormatId = FName(TEXT("Format.Skirmish2v2"));
	M.bHexTopology = true;
	M.ObserverTeamIds = { 0, 1 };

	if (!TestTrue(TEXT("il turno si registra"), URTReplayRecorderLibrary::RecordTurn(Root, M, 1, Voci)))
	{
		return false;
	}

	auto ImportiVisti = [&](int32 ObserverTeamId, TArray<int32>& Out) -> bool
	{
		FRTReplaySession Sessione;
		const ERTReplayOpenResult Esito =
			URTReplayPlayerLibrary::OpenArchive(Root, M.MatchId, Sessione, ObserverTeamId);
		if (Esito != ERTReplayOpenResult::Opened || Sessione.Traces.Num() != 1)
		{
			return false;
		}
		Out = Importi(Sessione.Traces[0]);
		return true;
	};

	// --- Squadra 0: vede il proprio fatto e quello pubblico, NON quello della squadra 1 ----------------
	TArray<int32> Squadra0;
	if (!TestTrue(TEXT("l'archivio si apre con gli occhi della squadra 0"), ImportiVisti(0, Squadra0)))
	{
		return false;
	}
	TestTrue(TEXT("la squadra 0 vede il proprio fatto"), Squadra0.Contains(10));
	TestTrue(TEXT("e vede il fatto pubblico"), Squadra0.Contains(30));
	TestFalse(TEXT("ma NON vede il fatto che solo la squadra 1 conosceva"), Squadra0.Contains(20));
	TestEqual(TEXT("due voci e non tre"), Squadra0.Num(), 2);

	// --- Squadra 1: speculare. Senza questa meta' un filtro cablato su «nascondi 20» passerebbe ---------
	TArray<int32> Squadra1;
	if (!TestTrue(TEXT("l'archivio si apre con gli occhi della squadra 1"), ImportiVisti(1, Squadra1)))
	{
		return false;
	}
	TestTrue(TEXT("la squadra 1 vede il proprio fatto"), Squadra1.Contains(20));
	TestTrue(TEXT("e vede il fatto pubblico"), Squadra1.Contains(30));
	TestFalse(TEXT("ma NON vede il fatto che solo la squadra 0 conosceva"), Squadra1.Contains(10));
	TestEqual(TEXT("due voci e non tre"), Squadra1.Num(), 2);

	// --- Lo spettatore NEUTRALE: risposta dichiarata da [D-316] punto (5) ------------------------------
	TArray<int32> Neutrale;
	if (!TestTrue(TEXT("l'archivio si apre come spettatore neutrale"), ImportiVisti(INDEX_NONE, Neutrale)))
	{
		return false;
	}
	TestEqual(TEXT("il neutrale vede tutte e tre le voci: e' la scelta, non una falla"), Neutrale.Num(), 3);

	// --- Una squadra che l'archivio non conosce ricade sulla canonica, e non su un file mancante -------
	// ⚠️ E' il comportamento di OGNI manifest `v1`, cioe' di ogni partita registrata prima di [D-316].
	TArray<int32> Sconosciuta;
	if (!TestTrue(TEXT("una squadra non dichiarata apre comunque l'archivio"), ImportiVisti(7, Sconosciuta)))
	{
		return false;
	}
	TestEqual(TEXT("e riceve la traccia canonica, come prima di D-316"), Sconosciuta.Num(), 3);

	RTReplayFixtures::Pulisci(Root);
	return true;
}

/**
 * **La traccia canonica NON cambia** quando l'archivio porta anche quelle per osservatore ([D-316]).
 *
 * ⚠️ E' la meta' che protegge il determinismo: `OrderedHashPerTurn` e i confronti fra tracce si appoggiano
 * alla canonica, e un recorder che l'avesse filtrata «tanto poi c'e' quella completa nell'audit» avrebbe
 * reso l'hash del manifest una funzione di quante squadre giocavano. Il difetto sarebbe comparso come una
 * divergenza inspiegabile mesi dopo, non come un test rosso.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacyCanonicalUnchangedTest,
	"RefactorTactics.Replay.Privacy.ObserverTracesLeaveTheCanonicalOneIntact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacyCanonicalUnchangedTest::RunTest(const FString&)
{
	const FString Root = RTReplayFixtures::TransientRoot(TEXT("CanonicalIntact"));
	RTReplayFixtures::Pulisci(Root);

	TArray<FRTTurnLogEntry> Voci;
	Voci.Add(VoceConVerdetto(10, SoloSquadra(0)));
	Voci.Add(VoceConVerdetto(20, SoloSquadra(1)));
	URTTurnLogLibrary::SortTurnLog(Voci);

	FRTReplayManifest Senza;
	Senza.MatchId = FGuid::NewGuid();
	Senza.FormatId = FName(TEXT("Format.Skirmish2v2"));
	Senza.bHexTopology = true;
	// `ObserverTeamIds` vuoto: e' un archivio come quelli scritti prima di [D-316].

	FRTReplayManifest Con = Senza;
	Con.MatchId = FGuid::NewGuid();
	Con.ObserverTeamIds = { 0, 1 };

	TestTrue(TEXT("si registra senza tracce per osservatore"),
		URTReplayRecorderLibrary::RecordTurn(Root, Senza, 1, Voci));
	TestTrue(TEXT("e si registra con"), URTReplayRecorderLibrary::RecordTurn(Root, Con, 1, Voci));

	// L'hash ordinato del turno e' lo STESSO: le tracce per osservatore sono un derivato, e non entrano in
	// nessun hash di determinismo.
	if (Senza.OrderedHashPerTurn.Num() == 1 && Con.OrderedHashPerTurn.Num() == 1)
	{
		TestEqual(TEXT("l'hash ordinato non dipende da quante squadre osservano"),
			Con.OrderedHashPerTurn[0], Senza.OrderedHashPerTurn[0]);
	}
	else
	{
		AddError(TEXT("un hash per turno per ciascun archivio"));
	}

	// E i byte della canonica sono identici nei due archivi.
	auto ByteCanonici = [&](const FRTReplayManifest& Manifest, TArray<uint8>& Out) -> bool
	{
		const FString Percorso = FPaths::Combine(
			URTReplayRecorderLibrary::MatchDirectory(Root, Manifest.MatchId),
			URTReplayRecorderLibrary::TurnFileName(1));
		return FFileHelper::LoadFileToArray(Out, *Percorso);
	};

	TArray<uint8> ByteSenza;
	TArray<uint8> ByteCon;
	if (TestTrue(TEXT("la canonica esiste in entrambi"),
			ByteCanonici(Senza, ByteSenza) && ByteCanonici(Con, ByteCon)))
	{
		TestTrue(TEXT("ed e' byte-identica"), ByteSenza == ByteCon);
	}

	// ⛔ **La canonica NON e' filtrata**: contiene ancora entrambe le voci. Se un giorno diventasse il
	// prodotto per osservatore, questo assert lo direbbe invece di lasciarlo scoprire a un audit.
	TArray<FRTTurnLogEntry> Rilette;
	ERTLogTopology Topologia = ERTLogTopology::Square;
	const FString PercorsoCanonico = FPaths::Combine(
		URTReplayRecorderLibrary::MatchDirectory(Root, Con.MatchId),
		URTReplayRecorderLibrary::TurnFileName(1));
	if (TestTrue(TEXT("la canonica si rilegge"),
			URTTurnLogLibrary::LoadTurnLogFromFile(PercorsoCanonico, Rilette, &Topologia)))
	{
		TestEqual(TEXT("e porta ancora i fatti di entrambe le squadre"), Rilette.Num(), 2);
	}

	RTReplayFixtures::Pulisci(Root);
	return true;
}

// =====================================================================================================
// SONDA — le celle di un TERZO in una traccia pubblica ([D-371] / `BLIND-1`, ereditata il 2026-09-10)
// =====================================================================================================

/**
 * ⚠️ **Quello che segue e' una SONDA, non un gate**, e la differenza governa come si legge.
 *
 * I sette test qui sopra sono gate: diventano rossi quando qualcuno rompe una proprieta' che vale. Questo
 * e' **verde perche' il canale c'e'** — misura un difetto aperto, nella forma che
 * `Tests/RTBlindActionsLeakMeasureTests.cpp` ha gia' usato per `BLIND-1` e `BLIND-4`. Il suo mestiere e'
 * togliere la domanda dal terreno dell'opinione: quanto grande sia il divario fra il prodotto pubblico e
 * la traccia privata si misura, non si asserisce.
 *
 * 🔴 **Da dove viene la domanda.** [`D-371`] ha chiuso `BLIND-1` il 2026-09-10 con l'uscita *(c)*:
 * l'occupazione autorevole **non** e' informazione pubblica, e il filtro vale anche per il bot. La stessa
 * voce di `docs/OPEN_DECISIONS.md` assegna per nome il residuo — *«`SrcCell`, `TgtCell` e
 * `SightBlockerCell` sono `Public`. Se `BLIND-1` uscisse (b) o (c), quei campi continuerebbero a
 * raccontare posizioni autorevoli in una traccia pubblica […] una domanda da porre a `#1805` e a
 * [`D-316`]»*. E' uscita *(c)*. Questa e' la misura di quella domanda.
 *
 * 🔑 **I due confini di `#1805` rispondono a meta' domanda ciascuno, e la meta' che manca e' la stessa.**
 * `FilterEntriesForObserver` chiede *«posso vedere questo SOGGETTO?»* e il verdetto e' congelato contro
 * **uno solo** (`FRTVerdictSubjectRef`, `Turn/RTTurnLog.h`); `ToPublicTrace` chiede *«questa COLONNA e'
 * pubblica?»* e `SrcCell` lo e' (`Replay/RTReplayPrivacyLibrary.cpp:32`). Nessuno dei due chiede *«di chi
 * e' la cella che questa voce nomina?»*, e su ogni voce che nomina due unita' le domande divergono.
 *
 * ⚠️ **Il repository lo sa gia', per un produttore solo.** `URTFacingLibrary::MakeHitCameFromSideEntry`
 * scrive il difensore in ENTRAMBE le celle, e il suo docstring dichiara perche': *«scriverci l'origine
 * pubblicherebbe la cella esatta di un attaccante che il lettore potrebbe non percepire, su OGNI colpo
 * risolto **invece che sui rari bypass**»* (`Turn/RTFacingLibrary.h`). L'ultima clausola e' un'eccezione
 * accettata, non una chiusura: sui bypass il canale resta, e nessuna voce `D-` lo dichiara.
 *
 * ⛔ **Questa sonda non sceglie il rimedio, e non deve.** Sbiancare `SrcCell` renderebbe rosso
 * `PublicFieldsKeepTheirValue`; marcarla `AuditOnly` svuoterebbe il prodotto pubblico; cambiare i
 * produttori riscrive `SrcCell` su voci gia' archiviate e rigenera i golden. Sono tre costi diversi per
 * una decisione d'autore che [D-371] ha reso necessaria e non ha preso.
 *
 * 🔑 **Diventa ROSSA** il giorno in cui quella decisione e' presa e implementata in un verso qualunque che
 * chiuda il canale — non prima. Chi la trova rossa senza aver toccato la privacy delle celle ha trovato
 * un'altra cosa, e deve leggere questo blocco prima di «aggiustarla».
 */
namespace
{
	/**
	 * Una voce nella forma esatta di `ERTFacingOutcome::RearHitBypassedCover` come la scrive il resolver:
	 * **`SrcCell` e' l'attaccante, il soggetto del verdetto e' la VITTIMA**
	 * (`Turn/RTTurnManager.cpp`, il ramo `Hit.CoverBypassedByFacing > 0` che chiude con
	 * `AppendLogEntry(BypassedCover, Victim)`).
	 *
	 * ⚠️ Costruita a mano e non ottenuta dal resolver, di proposito: il canale sta nella FORMA della voce —
	 * una cella pubblica che appartiene a un'unita' diversa dal soggetto — e farla nascere da una partita
	 * la legherebbe alla semantica di `FRTHexHit`, cioe' a `Combat/`, senza misurare niente di piu'.
	 */
	FRTTurnLogEntry ColpoAlleSpalle(const FRTCellId& CellaAttaccante, const FRTKnowledgeVerdict& Verdetto)
	{
		FRTTurnLogEntry E;
		E.Phase = ERTMatchPhase::Blast;
		E.Category = ERTLogCategory::Facing;
		E.Outcome = static_cast<uint8>(ERTFacingOutcome::RearHitBypassedCover);
		E.TurnNumber = 1;
		// La VITTIMA: e' lei il soggetto, ed e' lei che `UnitId` nomina.
		E.UnitId = 7;
		E.TgtCell = FRTCellId(0, 0);
		// L'ATTACCANTE: la cella di un'unita' che il verdetto non rappresenta.
		E.SrcCell = CellaAttaccante;
		E.Amount = 2;
		E.Verdict = Verdetto;
		return E;
	}

	/** Le `SrcCell` presenti, nell'ordine. */
	TArray<FRTCellId> CelleSorgente(const TArray<FRTTurnLogEntry>& Voci)
	{
		TArray<FRTCellId> Out;
		for (const FRTTurnLogEntry& E : Voci) { Out.Add(E.SrcCell); }
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacyThirdPartyCellTest,
	"RefactorTactics.Replay.Privacy.PublicCellsLeakAThirdPartyPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacyThirdPartyCellTest::RunTest(const FString&)
{
	const FString Root = RTReplayFixtures::TransientRoot(TEXT("ThirdPartyCell"));
	RTReplayFixtures::Pulisci(Root);

	// Due mondi identici in tutto cio' che la squadra 0 e' AUTORIZZATA a sapere, e diversi solo per dove
	// stava l'attaccante — che la squadra 0 non percepisce. E' la forma dell'invariante di `#2792`:
	//   ObserverKnowledge(A) == ObserverKnowledge(B)  =>  vista osservabile(A) == vista osservabile(B)
	const FRTCellId AttaccanteInA(3, 0);
	const FRTCellId AttaccanteInB(-3, 2);

	// Il verdetto e' lo STESSO nei due mondi, ed e' il punto: la vittima e' della squadra 0, quindi
	// `ClassifyTarget` risponde `Allowed` per corto-circuito («un alleato non passa dalla conoscenza») e il
	// bit si accende sempre. Nessuna delle due esecuzioni dice alla squadra 0 qualcosa sull'attaccante —
	// eppure la sua cella arriva.
	const FRTKnowledgeVerdict SoggettoVittima = SoloSquadra(0);

	// ⛔ CONTROLLO POSITIVO: una voce il cui SOGGETTO e' l'attaccante. Qui il filtro per voci ha la domanda
	// che sa rispondere, e deve togliere la riga alla squadra 0. Senza questa meta', una sonda che trovasse
	// «la squadra 0 legge tutto» non distinguerebbe un canale aperto da un filtro rotto.
	const FRTKnowledgeVerdict SoggettoAttaccante = SoloSquadra(1);

	auto Archivia = [&](const FRTCellId& CellaAttaccante, FRTReplayManifest& OutManifest) -> bool
	{
		TArray<FRTTurnLogEntry> Voci;
		Voci.Add(ColpoAlleSpalle(CellaAttaccante, SoggettoVittima));
		Voci.Add(VoceConVerdetto(99, SoggettoAttaccante));
		URTTurnLogLibrary::SortTurnLog(Voci);

		OutManifest.MatchId = FGuid::NewGuid();
		OutManifest.FormatId = FName(TEXT("Format.Skirmish2v2"));
		OutManifest.bHexTopology = true;
		OutManifest.ObserverTeamIds = { 0, 1 };
		return URTReplayRecorderLibrary::RecordTurn(Root, OutManifest, 1, Voci);
	};

	FRTReplayManifest MondoA;
	FRTReplayManifest MondoB;
	if (!TestTrue(TEXT("i due mondi si registrano"), Archivia(AttaccanteInA, MondoA) && Archivia(AttaccanteInB, MondoB)))
	{
		return false;
	}

	auto VistaSquadra0 = [&](const FRTReplayManifest& Manifest, TArray<FRTTurnLogEntry>& Out) -> bool
	{
		FRTReplaySession Sessione;
		const ERTReplayOpenResult Esito =
			URTReplayPlayerLibrary::OpenArchive(Root, Manifest.MatchId, Sessione, /*ObserverTeamId*/ 0);
		if (Esito != ERTReplayOpenResult::Opened || Sessione.Traces.Num() != 1)
		{
			return false;
		}
		Out = Sessione.Traces[0];
		return true;
	};

	TArray<FRTTurnLogEntry> VisteA;
	TArray<FRTTurnLogEntry> VisteB;
	if (!TestTrue(TEXT("la squadra 0 apre entrambi gli archivi"),
			VistaSquadra0(MondoA, VisteA) && VistaSquadra0(MondoB, VisteB)))
	{
		return false;
	}

	// --- Il filtro per VOCI funziona, e va misurato prima del resto ------------------------------------
	// Se questa meta' cadesse, tutto il resto misurerebbe un filtro rotto invece del canale.
	TestEqual(TEXT("la squadra 0 riceve una voce sola: quella di cui e' soggetto"), VisteA.Num(), 1);
	TestEqual(TEXT("e lo stesso nell'altro mondo"), VisteB.Num(), 1);
	TestFalse(TEXT("CONTROLLO POSITIVO: la voce il cui soggetto e' l'attaccante NON arriva"),
		Importi(VisteA).Contains(99));

	if (VisteA.Num() != 1 || VisteB.Num() != 1)
	{
		return false;
	}

	// --- E nonostante quel filtro, la cella dell'attaccante e' nella traccia della squadra 0 -----------
	TestEqual(TEXT("MISURA: la traccia per osservatore porta la cella dell'attaccante del mondo A"),
		VisteA[0].SrcCell, AttaccanteInA);
	TestEqual(TEXT("e quella del mondo B"), VisteB[0].SrcCell, AttaccanteInB);
	// ⚠️ `TestTrue` e non `TestNotEqual`: l'overload generico di `TestNotEqual` non esiste in UE 5.8 —
	// `AutomationTest.h` ne dichiara solo per stringhe, `FText` e `FName`, mentre il `TestEqual` templato
	// c'e' (e usa `ToString()`). I valori vanno quindi nel messaggio a mano.
	TestTrue(*FString::Printf(
			TEXT("🔴 IL CANALE: conoscenza autorizzata identica, traccia osservabile DIVERSA — %s contro %s"),
			*VisteA[0].SrcCell.ToString(), *VisteB[0].SrcCell.ToString()),
		VisteA[0].SrcCell != VisteB[0].SrcCell);

	// --- La seconda uscita: la superficie spettatore, dove il confine dei CAMPI e' l'unico in servizio --
	// ⚠️ Vanno misurate entrambe. Il file per osservatore e `ToPublicTrace` sono due prodotti distinti, e
	// una correzione applicata a uno solo lascia l'altro aperto.
	const TArray<FRTPublicReplayEntry> PubblicheA = URTReplayPrivacyLibrary::ToPublicTrace(VisteA);
	const TArray<FRTPublicReplayEntry> PubblicheB = URTReplayPrivacyLibrary::ToPublicTrace(VisteB);
	if (TestEqual(TEXT("il ponte pubblico conserva la voce"), PubblicheA.Num(), 1)
		&& TestEqual(TEXT("in entrambi i mondi"), PubblicheB.Num(), 1))
	{
		TestEqual(TEXT("MISURA: e la cella dell'attaccante esce anche dal prodotto pubblico"),
			PubblicheA[0].SrcCell, AttaccanteInA);
		TestTrue(TEXT("🔴 IL CANALE, sulla superficie spettatore"),
			PubblicheA[0].SrcCell != PubblicheB[0].SrcCell);
	}

	AddInfo(FString::Printf(
		TEXT("Canale misurato: la squadra 0 riceve 1 voce su 2 (il filtro per VOCI regge), e quella voce ")
		TEXT("porta SrcCell=(%d,%d,L%d) nel mondo A contro (%d,%d,L%d) nel mondo B — la posizione di ")
		TEXT("un'unita' che la squadra 0 non percepisce, su entrambe le uscite del prodotto pubblico."),
		AttaccanteInA.X, AttaccanteInA.Y, AttaccanteInA.Layer,
		AttaccanteInB.X, AttaccanteInB.Y, AttaccanteInB.Layer));

	RTReplayFixtures::Pulisci(Root);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
