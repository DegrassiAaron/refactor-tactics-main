#include "RTLauncherScenarioBrowser.h"

#include "ScenarioHarness/RTScenarioDraft.h"
#include "ScenarioHarness/RTScenarioKnowledge.h"
#include "Turn/RTTurnRules.h" // ERTMatchPhase, per il nome leggibile della fase

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

FText FRTLauncherScenarioBrowser::DescribePerspective(int32 TeamId)
{
	if (TeamId == RTScenarioKnowledge::OmniscientTeamId)
	{
		// Non tradotta: e' il nome della posizione, ed e' lo stesso che la issue, la documentazione e il
		// codice usano. Chiamarla «tutti» in italiano la farebbe leggere come l'assenza di una scelta.
		return NSLOCTEXT("RTLauncherScenarioBrowser", "PerspectiveOmniscient", "Omniscient");
	}

	// L'id della squadra, non la posizione nel selettore: e' cosi' che si confronta con
	// `rt.Debug.Knowledge <team>`, che e' l'oracolo gia' esistente.
	return FText::Format(NSLOCTEXT("RTLauncherScenarioBrowser", "PerspectiveTeam", "Team {0}"),
		FText::AsNumber(TeamId));
}

FString FRTLauncherScenarioBrowser::CoinUnitId(const TArray<FRTScenarioUnitView>& Existing)
{
	// ⚠️ Il confronto e' **case-insensitive**: `FRTScenarioDraft` non impone una capitalizzazione agli id, e
	// un `u1` gia' schierato renderebbe `U1` una collisione per il draft e un id libero per questa
	// funzione. Sbagliare in questo verso produce esattamente il messaggio che il conio esiste per evitare.
	TSet<FString> Taken;
	Taken.Reserve(Existing.Num());
	for (const FRTScenarioUnitView& Unit : Existing)
	{
		Taken.Add(Unit.Id.ToLower());
	}

	// Il limite superiore e' `Num() + 1`: con N id presi, fra `U1` e `U(N+1)` almeno uno e' libero per il
	// principio della piccionaia. Il ciclo termina sempre, e non serve una guardia sul numero di giri.
	for (int32 Candidate = 1; Candidate <= Existing.Num() + 1; ++Candidate)
	{
		const FString Proposed = FString::Printf(TEXT("U%d"), Candidate);
		if (!Taken.Contains(Proposed.ToLower()))
		{
			return Proposed;
		}
	}

	// Irraggiungibile per l'argomento sopra. Restituire una stringa vuota invece di un id inventato: la
	// facade la rifiuta con `Invalid`, che e' un esito visibile — un id casuale finirebbe nel file.
	return FString();
}

FText FRTLauncherScenarioBrowser::DescribePlaybackPosition(const FRTReplayPosition& Position)
{
	// 🔴 **`Ended` e `BeforeStart` sono DUE stati diversi**, e `HasTurn()` e' falso in entrambi: la
	// prima stesura guardava solo quello e scriveva *«Posa iniziale»* anche a partita finita. Si vedeva
	// subito — bastava premere `>` fino in fondo — ma nessun automation test poteva accorgersene, perche'
	// questa era una stringa di presentazione dentro un pannello Slate. Trovato via MCP il 2026-09-04.
	// Da #2788 la funzione vive qui, e quel test esiste.
	if (Position.State == ERTReplayPositionState::Ended)
	{
		return LOCTEXT("PlaybackAtEnd", "Fine della risoluzione.");
	}

	if (!Position.HasTurn())
	{
		// Restano `BeforeStart` e `Unaddressable`. Il secondo porta una fase leggibile ma nessun turno:
		// dirlo e' meglio che tacerlo, perche' altrimenti si legge come l'inizio.
		if (Position.HasPhase())
		{
			return LOCTEXT("PlaybackUnaddressable", "Posizione non raggiungibile nella traccia.");
		}
		return LOCTEXT("PlaybackAtStart", "Posa iniziale.");
	}

	const UEnum* TipoFase = StaticEnum<ERTMatchPhase>();
	const FText Fase = TipoFase
		? TipoFase->GetDisplayNameTextByValue(static_cast<int64>(Position.Phase))
		: FText::GetEmpty();

	return FText::Format(LOCTEXT("PlaybackAt", "Turno {0} · {1}"),
		FText::AsNumber(Position.TurnNumber), Fase);
}

FText FRTLauncherScenarioBrowser::DescribeTransport(const FRTLauncherTransportStatus& Status)
{
	// I turni come argomento NUMERICO e non come testo gia' formattato: e' cio' che rende usabile
	// `|plural(...)`. «1 turni» su `Movement.Basic` — che di turno ne ha esattamente uno — sarebbe la prima
	// cosa che si nota, e la meno interessante.
	FFormatNamedArguments Argomenti;
	Argomenti.Add(TEXT("Turni"), Status.TurnsPlayed);

	if (Status.bPlaybackOpen)
	{
		if (Status.Run != ERTLauncherRunState::Ran)
		{
			// ⚠️ Un playback aperto di cui questo pannello non ha memoria — ricostruito dopo che la corsa
			// era gia' stata lanciata, per esempio. Si dice dove si e' e **non** «corsa di 0 turni», che
			// sarebbe un conteggio inventato su una traccia che invece ne ha.
			return DescribePlaybackPosition(Status.Position);
		}

		// 🔑 **Qui sta la distinzione che #2788 chiede.** La stessa `Posa iniziale.` che prima si leggeva
		// come «non e' successo niente» ora arriva preceduta dalla corsa che l'ha prodotta: il campo mostra
		// il turno 0 **di una traccia**, non lo schieramento d'authoring.
		Argomenti.Add(TEXT("Posizione"), DescribePlaybackPosition(Status.Position));
		return FText::Format(
			LOCTEXT("TransportRan", "Corsa di {Turni} {Turni}|plural(one=turno,other=turni) · {Posizione}"),
			Argomenti);
	}

	// ⚠️ Nessun `default:`, come in `DescribeEmptyState` e per la stessa ragione: uno stato aggiunto domani
	// deve rompere la compilazione qui, non tradursi in silenzio nella frase di un altro.
	switch (Status.Run)
	{
	case ERTLauncherRunState::NotRun:
		return LOCTEXT("TransportNoPlayback", "Nessun playback: esegui uno scenario.");

	case ERTLauncherRunState::Failed:
		// Manda a leggere, invece di invitare a ripetere il gesto: il messaggio della facade e' nel readout.
		return LOCTEXT("TransportFailed", "Corsa fallita: nessun playback. Il referto dice perche'.");

	case ERTLauncherRunState::Ran:
		if (Status.TurnsPlayed == 0)
		{
			// 🎯 **Il caso che ha ingannato un lettore.** Una corsa senza turni non apre nessun playback, e
			// la riga diceva «esegui uno scenario» a chi lo aveva appena eseguito. Non e' un errore: e' un
			// esito legittimo, e va detto come tale.
			return LOCTEXT("TransportNoTurns", "Corsa eseguita: nessun turno da riprodurre.");
		}

		// Turni giocati ma niente da riprodurre: traccia non decodificabile, oppure aggregato a varianti,
		// che non porta ne' hash ne' TurnLog (`FRTScenarioRunReport::bHasTrace`). Dirlo separa il guasto
		// dello strumento dall'esito vuoto qui sopra.
		return FText::Format(
			LOCTEXT("TransportNotPlayable",
				"Corsa di {Turni} {Turni}|plural(one=turno,other=turni): traccia non riproducibile."),
			Argomenti);
	}

	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
