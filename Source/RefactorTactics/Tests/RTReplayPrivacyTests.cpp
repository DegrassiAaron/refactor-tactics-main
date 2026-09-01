#include "Misc/AutomationTest.h"
#include "Replay/RTReplayPrivacyLibrary.h"
#include "Replay/RTReplayViewerSubsystem.h"
#include "Turn/RTTurnLog.h"
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
		E.ActionId = FName(TEXT("Hero.Gadget.ArcPulse"));
		E.BaseActionId = FName(TEXT("Action.BasicAttack"));
		E.UnitId = 42;
		E.Verdict = FRTKnowledgeVerdict::Everyone();
		E.TurnNumber = 7;
		E.GraphRevision = 9;
		E.Priority = 55;
		E.OpportunityId = TEXT("OPP-1");
		E.ReactionInstanceId = 11;
		E.SelectedTargetUnitId = 43;
		E.OriginalTargetUnitId = 44;
		E.ReactionResponse = TEXT("FIRE");
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

#endif // WITH_DEV_AUTOMATION_TESTS
