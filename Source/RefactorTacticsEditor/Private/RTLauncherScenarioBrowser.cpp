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

ERTLauncherListState FRTLauncherScenarioBrowser::Classify(int32 FilteredCount, int32 VisibleCount, bool bAnyTagFilter)
{
	if (VisibleCount > 0)
	{
		return ERTLauncherListState::Populated;
	}

	if (FilteredCount == 0)
	{
		// L'ordine di questi due rami e' il contenuto della funzione. Un indice vuoto MENTRE nessun filtro
		// restringe non e' colpa dei filtri: dire «allarga» manderebbe a cercare una via d'uscita che non
		// esiste, e la causa (`Scenarios/` assente, header illeggibili) e' fuori dal pannello.
		return bAnyTagFilter
			? ERTLauncherListState::NoTagMatches
			: ERTLauncherListState::EmptyCorpus;
	}

	// I tag lasciavano passare qualcosa: allora e' stata la ricerca. Attribuire ai tag un vuoto causato
	// dalla ricerca — o viceversa — manda a cancellare la cosa sbagliata, e l'elenco resta vuoto lo stesso.
	return ERTLauncherListState::NoSearchMatches;
}

FText FRTLauncherScenarioBrowser::DescribeEmptyState(ERTLauncherListState State)
{
	// ⚠️ Nessun `default:`, e non e' pedanteria: e' un enum cha ha per unico scopo tenere distinte delle
	// cause. Con un `default` uno stato aggiunto domani si tradurrebbe in silenzio in «nessun messaggio»,
	// cioe' in un elenco vuoto che non dice piu' perche' — il difetto che l'enum esiste per impedire. Senza,
	// il compilatore indica l'unico punto che va aggiornato.
	switch (State)
	{
	case ERTLauncherListState::Populated:
		// Il posto e' occupato dalla lista. Restituire qui una stringa qualsiasi la farebbe comparire
		// sotto un elenco pieno.
		return FText::GetEmpty();

	case ERTLauncherListState::EmptyCorpus:
		return LOCTEXT("EmptyCorpus", "L'indice degli scenari e' vuoto: non c'e' niente da filtrare. Controlla che la cartella Scenarios/ del progetto sia raggiungibile.");

	case ERTLauncherListState::NoTagMatches:
		// «i tag scelti» e non «entrambi i tag»: le tendine sono due ma se ne puo' usare una sola, e in quel
		// caso una frase al plurale manda a cercare un secondo filtro che nessuno ha impostato.
		return LOCTEXT("NoTagMatches", "Nessuno scenario porta i tag scelti. Allarga i filtri.");

	case ERTLauncherListState::NoSearchMatches:
		return LOCTEXT("NoSearchMatches", "I filtri lasciano passare degli scenari, ma nessuno contiene il testo cercato.");
	}

	return FText::GetEmpty();
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

	// ⚠️ Ne' l'uno ne' l'altro. Il corpus oggi non ha questo caso — misurato il 2026-08-30: 90 scenari,
	// 21 con una fixture e 69 con un raggio, nessuno senza — ma un readout che
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
		// Solo quando ce ne sono: una riga «varianti 0» sui novanta scenari, quasi tutti senza varianti, e' rumore
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
