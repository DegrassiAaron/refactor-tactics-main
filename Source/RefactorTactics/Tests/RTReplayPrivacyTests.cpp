#include "Misc/AutomationTest.h"
#include "Replay/RTReplayPrivacyLibrary.h"
#include "Turn/RTTurnLog.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Il confine fra i DUE prodotti di [D-276]: il Replay Pubblico/Sanitizzato e la Traccia di Audit Privata.
 *
 * ⚠️ **Questi test non guardano dei valori, guardano dei CAMPI.** E' la differenza fra un gate che
 * dimostra che oggi non c'e' un leak e uno che dimostra che domani non ci puo' essere: il primo si
 * scrive con un assert su una voce, e resta verde quando qualcuno aggiunge il diciannovesimo campo.
 */
namespace
{
	/** I nomi delle `UPROPERTY` di una struct riflessa. */
	TSet<FName> NomiRiflessi(const UStruct* Tipo)
	{
		TSet<FName> Out;
		for (TFieldIterator<FProperty> It(Tipo); It; ++It)
		{
			Out.Add(It->GetFName());
		}
		return Out;
	}

	FString Elenco(const TSet<FName>& Nomi)
	{
		TArray<FString> Come;
		for (const FName& N : Nomi) { Come.Add(N.ToString()); }
		Come.Sort();
		return FString::Join(Come, TEXT(", "));
	}

	/** Una voce con un valore DIVERSO da quello di default in ogni campo, audit compresi. */
	FRTTurnLogEntry VoceSatura()
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
	const TMap<FName, ERTReplayFieldVisibility>& Tabella = URTReplayPrivacyLibrary::FieldVisibility();
	const TSet<FName> Riflessi = NomiRiflessi(FRTTurnLogEntry::StaticStruct());

	// Anti-vacuita': una tabella vuota renderebbe verdi i due controlli sotto per assenza di soggetto.
	TestTrue(TEXT("la tabella classifica almeno un campo"), Tabella.Num() > 0);
	TestTrue(TEXT("la reflection vede almeno un campo di FRTTurnLogEntry"), Riflessi.Num() > 0);

	TSet<FName> NonClassificati;
	for (const FName& N : Riflessi)
	{
		if (!Tabella.Contains(N)) { NonClassificati.Add(N); }
	}
	TestTrue(
		FString::Printf(TEXT("ogni campo di FRTTurnLogEntry e' classificato; non classificati: [%s]"),
			*Elenco(NonClassificati)),
		NonClassificati.Num() == 0);

	// Il difetto simmetrico: un campo rinominato lascia nella tabella un nome che non esiste piu', e la
	// classificazione smette di riguardare qualcosa senza che nessuno se ne accorga.
	TSet<FName> Fantasmi;
	for (const TPair<FName, ERTReplayFieldVisibility>& Voce : Tabella)
	{
		if (!Riflessi.Contains(Voce.Key)) { Fantasmi.Add(Voce.Key); }
	}
	TestTrue(
		FString::Printf(TEXT("la tabella non classifica campi inesistenti; fantasmi: [%s]"), *Elenco(Fantasmi)),
		Fantasmi.Num() == 0);

	return true;
}

/**
 * La biiezione fra la classificazione e il tipo pubblico, che chiude i due difetti opposti:
 * un campo di audit che scivola nel prodotto pubblico, e un campo classificato pubblico che non viene
 * mai esportato — il secondo non e' un leak, ma e' una classificazione che mente.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacyPublicTypeTest,
	"RefactorTactics.Replay.Privacy.PublicEntryMatchesTheClassification",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacyPublicTypeTest::RunTest(const FString&)
{
	const TMap<FName, ERTReplayFieldVisibility>& Tabella = URTReplayPrivacyLibrary::FieldVisibility();
	const TSet<FName> NelTipoPubblico = NomiRiflessi(FRTPublicReplayEntry::StaticStruct());

	TSet<FName> ClassificatiPubblici;
	TSet<FName> ClassificatiAudit;
	for (const TPair<FName, ERTReplayFieldVisibility>& Voce : Tabella)
	{
		(Voce.Value == ERTReplayFieldVisibility::Public ? ClassificatiPubblici : ClassificatiAudit).Add(Voce.Key);
	}

	TestTrue(TEXT("almeno un campo e' classificato pubblico"), ClassificatiPubblici.Num() > 0);
	TestTrue(TEXT("almeno un campo e' classificato audit-only"), ClassificatiAudit.Num() > 0);

	const TSet<FName> AuditNelPubblico = NelTipoPubblico.Intersect(ClassificatiAudit);
	TestTrue(
		FString::Printf(TEXT("nessun campo audit-only vive dentro FRTPublicReplayEntry; trovati: [%s]"),
			*Elenco(AuditNelPubblico)),
		AuditNelPubblico.Num() == 0);

	const TSet<FName> PubbliciMancanti = ClassificatiPubblici.Difference(NelTipoPubblico);
	TestTrue(
		FString::Printf(TEXT("ogni campo classificato pubblico esiste nel tipo pubblico; mancanti: [%s]"),
			*Elenco(PubbliciMancanti)),
		PubbliciMancanti.Num() == 0);

	const TSet<FName> PubbliciDiTroppo = NelTipoPubblico.Difference(ClassificatiPubblici);
	TestTrue(
		FString::Printf(TEXT("il tipo pubblico non porta campi fuori dalla classificazione; di troppo: [%s]"),
			*Elenco(PubbliciDiTroppo)),
		PubbliciDiTroppo.Num() == 0);

	return true;
}

/**
 * Il ponte copia i campi pubblici **con il loro valore** e non porta con se' niente altro.
 *
 * La prima meta' serve piu' della seconda: un sanitizer che azzerasse tutto passerebbe qualunque test
 * di privacy e non sarebbe un replay.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacySanitizeTest,
	"RefactorTactics.Replay.Privacy.SanitizeDropsAuditFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacySanitizeTest::RunTest(const FString&)
{
	const FRTTurnLogEntry Audit = VoceSatura();
	const TArray<FRTPublicReplayEntry> Pubblico = URTReplayPrivacyLibrary::ToPublicTrace({ Audit });

	if (!TestEqual(TEXT("una voce di audit produce una voce pubblica"), Pubblico.Num(), 1))
	{
		return false;
	}

	const FRTPublicReplayEntry& P = Pubblico[0];
	TestEqual(TEXT("Phase conservata"), static_cast<uint8>(P.Phase), static_cast<uint8>(Audit.Phase));
	TestEqual(TEXT("Category conservata"), static_cast<uint8>(P.Category), static_cast<uint8>(Audit.Category));
	TestEqual(TEXT("Outcome conservato"), static_cast<int32>(P.Outcome), static_cast<int32>(Audit.Outcome));
	TestTrue(TEXT("SrcCell conservata"), P.SrcCell == Audit.SrcCell);
	TestTrue(TEXT("TgtCell conservata"), P.TgtCell == Audit.TgtCell);
	TestEqual(TEXT("Amount conservato"), P.Amount, Audit.Amount);
	TestTrue(TEXT("ActionId conservato"), P.ActionId == Audit.ActionId);
	TestTrue(TEXT("BaseActionId conservato"), P.BaseActionId == Audit.BaseActionId);
	TestEqual(TEXT("UnitId conservato"), P.UnitId, Audit.UnitId);
	TestEqual(TEXT("TurnNumber conservato"), P.TurnNumber, Audit.TurnNumber);
	TestEqual(TEXT("GraphRevision conservata"), P.GraphRevision, Audit.GraphRevision);

	return true;
}

/**
 * Determinismo: nessun riordino, nessuna sorgente di variazione fra due chiamate (`D-263`).
 *
 * L'ordine non e' un dettaglio di comodo: la traccia arriva gia' ordinata da `SortTurnLog`, e un
 * sanitizer che riordinasse produrrebbe un replay che non e' quello che la partita ha risolto.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTReplayPrivacyDeterminismTest,
	"RefactorTactics.Replay.Privacy.SanitizeIsOrderPreservingAndPure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTReplayPrivacyDeterminismTest::RunTest(const FString&)
{
	TArray<FRTTurnLogEntry> Audit;
	for (int32 i = 0; i < 8; ++i)
	{
		FRTTurnLogEntry E = VoceSatura();
		E.Amount = i;          // pubblico: distingue le voci nel prodotto
		E.Priority = 100 - i;  // audit: non deve arrivare, e non deve nemmeno ordinare
		Audit.Add(E);
	}

	const TArray<FRTPublicReplayEntry> Prima = URTReplayPrivacyLibrary::ToPublicTrace(Audit);
	const TArray<FRTPublicReplayEntry> Dopo = URTReplayPrivacyLibrary::ToPublicTrace(Audit);

	TestEqual(TEXT("il conteggio si conserva"), Prima.Num(), Audit.Num());

	bool bOrdinePreservato = Prima.Num() == Audit.Num();
	for (int32 i = 0; bOrdinePreservato && i < Prima.Num(); ++i)
	{
		bOrdinePreservato = Prima[i].Amount == Audit[i].Amount;
	}
	TestTrue(TEXT("l'ordine di ingresso e' quello di uscita"), bOrdinePreservato);

	bool bStessoRisultato = Prima.Num() == Dopo.Num();
	for (int32 i = 0; bStessoRisultato && i < Prima.Num(); ++i)
	{
		bStessoRisultato = Prima[i].Amount == Dopo[i].Amount && Prima[i].UnitId == Dopo[i].UnitId;
	}
	TestTrue(TEXT("due chiamate sullo stesso ingresso danno lo stesso risultato"), bStessoRisultato);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
