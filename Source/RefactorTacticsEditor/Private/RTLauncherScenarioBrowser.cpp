#include "RTLauncherScenarioBrowser.h"

#include "ScenarioHarness/RTScenarioDraft.h"

#define LOCTEXT_NAMESPACE "RTLauncherScenarioBrowser"

TArray<FString> FRTLauncherScenarioBrowser::ApplySearch(const TArray<FString>& FilteredIds, const FString& Search)
{
	// Ricerca vuota = identita'. Non e' un caso speciale da ricordare: e' cio' che rende «cercare a filtri
	// vuoti cerca su tutti» una conseguenza invece di una seconda strada nel codice.
	const FString Needle = Search.TrimStartAndEnd();
	if (Needle.IsEmpty())
	{
		return FilteredIds;
	}

	TArray<FString> Visible;
	Visible.Reserve(FilteredIds.Num());

	for (const FString& Id : FilteredIds)
	{
		// `Contains` e non `StartsWith`: gli id sono composti (`Movement.Basic`, `Reactions.Overwatch`) e
		// chi cerca `overwatch` non sa, e non deve sapere, sotto quale prefisso l'hanno messo.
		if (Id.Contains(Needle, ESearchCase::IgnoreCase))
		{
			Visible.Add(Id);
		}
	}

	return Visible;
}

ERTLauncherListState FRTLauncherScenarioBrowser::Classify(int32 FilteredCount, int32 VisibleCount)
{
	if (VisibleCount > 0)
	{
		return ERTLauncherListState::Populated;
	}

	// L'ordine dei due rami e' il contenuto della funzione. Se i tag non lasciano passare niente, la
	// ricerca non ha avuto nulla su cui lavorare e non e' lei la causa — anche quando c'e' del testo
	// nella casella. Attribuirla alla ricerca manderebbe a cancellare la parola sbagliata.
	return FilteredCount == 0
		? ERTLauncherListState::NoTagMatches
		: ERTLauncherListState::NoSearchMatches;
}

FText FRTLauncherScenarioBrowser::DescribeEmptyState(ERTLauncherListState State)
{
	switch (State)
	{
	case ERTLauncherListState::NoTagMatches:
		return LOCTEXT("NoTagMatches", "Nessuno scenario porta entrambi i tag. Allarga i filtri.");

	case ERTLauncherListState::NoSearchMatches:
		return LOCTEXT("NoSearchMatches", "I filtri lasciano passare degli scenari, ma nessuno contiene il testo cercato.");

	default:
		// `Populated` non ha messaggio: il posto e' occupato dalla lista. Restituire qui una stringa
		// qualsiasi la farebbe comparire sotto un elenco pieno.
		return FText::GetEmpty();
	}
}

FString FRTLauncherScenarioBrowser::DescribeTerrain(const FRTScenarioSummary& Summary)
{
	// L'allestimento vince quando c'e', perche' e' cio' che lo scenario ha dichiarato: `MapRadius` resta
	// al suo valore di default anche in uno scenario che parte da una fixture, e leggerlo li' significa
	// leggere un campo che nessuno ha scritto.
	if (!Summary.Fixture.IsEmpty())
	{
		return FString::Printf(TEXT("fixture %s"), *Summary.Fixture);
	}

	if (Summary.MapRadius > 0)
	{
		return FString::Printf(TEXT("radius %d"), Summary.MapRadius);
	}

	// ⚠️ Ne' l'uno ne' l'altro. Il corpus oggi non ha questo caso (21 + 67 = 88, tutti), ma un readout che
	// qui stampasse `radius 0` renderebbe un dato assente indistinguibile da un raggio davvero nullo.
	return TEXT("terreno non dichiarato");
}

FString FRTLauncherScenarioBrowser::DescribeComposition(const TArray<FRTScenarioUnitView>& Units)
{
	if (Units.Num() == 0)
	{
		return TEXT("nessuna unita' schierata");
	}

	// Mappa ordinata: le squadre escono per `TeamId` crescente a prescindere dall'ordine in cui le unita'
	// compaiono nel file. Due scenari con le stesse squadre devono leggersi uguali, altrimenti il readout
	// suggerisce una differenza che non c'e'.
	TMap<int32, int32> CountByTeam;
	for (const FRTScenarioUnitView& Unit : Units)
	{
		++CountByTeam.FindOrAdd(Unit.TeamId);
	}

	TArray<int32> TeamIds;
	CountByTeam.GetKeys(TeamIds);
	TeamIds.Sort();

	TArray<FString> Parts;
	Parts.Reserve(TeamIds.Num());
	for (const int32 TeamId : TeamIds)
	{
		Parts.Add(FString::Printf(TEXT("team %d: %d"), TeamId, CountByTeam[TeamId]));
	}

	return FString::Join(Parts, TEXT(" · "));
}

TArray<FString> FRTLauncherScenarioBrowser::BuildReadout(const FRTScenarioSummary& Summary, const TArray<FRTScenarioUnitView>& Units)
{
	TArray<FString> Lines;

	Lines.Add(FString::Printf(TEXT("terreno    %s"), *DescribeTerrain(Summary)));
	Lines.Add(FString::Printf(TEXT("squadre    %s"), *DescribeComposition(Units)));

	// I conteggi arrivano dal summary e non dagli array che abbiamo in mano: `UnitCount` e' cio' che lo
	// scenario dichiara, e se divergesse da `Units.Num()` la divergenza va vista, non nascosta contandola
	// una seconda volta qui.
	Lines.Add(FString::Printf(TEXT("unita'     %d"), Summary.UnitCount));
	Lines.Add(FString::Printf(TEXT("turni      %d"), Summary.TurnCount));
	Lines.Add(FString::Printf(TEXT("attese     %d"), Summary.ExpectationCount));

	if (Summary.VariantCount > 0)
	{
		// Solo quando ce ne sono: una riga «varianti 0» su 88 scenari quasi tutti senza varianti e' rumore
		// che allontana dall'occhio le righe che cambiano.
		Lines.Add(FString::Printf(TEXT("varianti   %d"), Summary.VariantCount));
	}

	if (Summary.Tags.Num() > 0)
	{
		// I tag come il file li scrive: la forma canonica serve ai filtri, non alla lettura. Mostrarli
		// normalizzati farebbe sembrare sbagliato il file a chi lo apre dopo.
		Lines.Add(FString::Printf(TEXT("tag        %s"), *FString::Join(Summary.Tags, TEXT(", "))));
	}

	return Lines;
}

#undef LOCTEXT_NAMESPACE
