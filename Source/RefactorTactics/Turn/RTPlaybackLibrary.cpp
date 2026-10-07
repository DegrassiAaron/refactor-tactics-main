#include "Turn/RTPlaybackLibrary.h"

FVector URTPlaybackLibrary::InterpolateAlongPath(const TArray<FVector>& Waypoints, float Alpha)
{
	const int32 N = Waypoints.Num();
	if (N == 0)
	{
		return FVector::ZeroVector;
	}
	if (N == 1)
	{
		return Waypoints[0];
	}

	const int32 SegCount = N - 1;
	const float T = FMath::Clamp(Alpha, 0.f, 1.f) * SegCount; // posizione continua in [0, SegCount]
	int32 Seg = FMath::FloorToInt(T);
	float Frac;
	if (Seg >= SegCount)
	{
		Seg = SegCount - 1; // Alpha == 1: ultimo segmento, completo
		Frac = 1.f;
	}
	else
	{
		Frac = T - Seg;
	}
	return FMath::Lerp(Waypoints[Seg], Waypoints[Seg + 1], Frac);
}

int32 URTPlaybackLibrary::AttacksToShow(int32 NumAttacks, float PhaseElapsed, float AttackShowSeconds)
{
	if (NumAttacks <= 0)
	{
		return 0;
	}
	if (AttackShowSeconds <= 0.f)
	{
		return NumAttacks; // nessuno scaglionamento richiesto: la fase li mostra tutti insieme
	}
	// Il primo colpo esce a fase appena iniziata (1 + ...): scaglionare non deve ritardare l'inizio,
	// altrimenti una fase con un colpo solo resterebbe muta per tutta AttackShowSeconds.
	const float Elapsed = FMath::Max(0.f, PhaseElapsed);
	return FMath::Min(NumAttacks, 1 + FMath::FloorToInt(Elapsed / AttackShowSeconds));
}

bool URTPlaybackLibrary::BlastPhaseIsActive(int32 NumAttacks, bool bHasBlastMove, int32 NumFootprints,
	int32 NumStructureHits, int32 NumActivations)
{
	// Cinque ragioni indipendenti; la quinta e' quella di #3549: un Blast di sole cure si vede. ⛔ Nessuna
	// somma e nessuna soglia: basta che UNA sia vera.
	return NumAttacks > 0 || bHasBlastMove || NumFootprints > 0 || NumStructureHits > 0 || NumActivations > 0;
}

float URTPlaybackLibrary::PhaseDuration(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumAttacks,
	float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds)
{
	// Una riga: la formula sta in `PhaseTime`, e il totale e' la somma dei suoi due termini. Non c'e' un
	// secondo calcolo da tenere allineato.
	// ⚠️ **TRE zeri, e nessuno e' una dimenticanza**: questo wrapper non conosce ne i colpi a struttura
	// (`#2828`), ne le impronte (`#3278`), ne le attivazioni (#3549). Passa `NumActivations = 0` e usa i soli
	// colpi come sequenza, quindi su un `Blast` la durata restituita e' SOTTOSTIMATA. ⛔ Chi dimensiona il
	// playback vero non passa di qui — `PhaseTimeForPlaybackPhase` chiama `PhaseTime` con la sequenza intera.
	// Questa forma sopravvive per i gate di pacing sulle fasi classiche. ⏱️ *Fino a #3549 diceva «DUE zeri».*
	return PhaseTime(Phase, MaxMoveSegments, /*NumActivations=*/ 0, /*NumSequenceElements=*/ NumAttacks,
		CellsPerSecond, AttackShowSeconds, PhaseBeatSeconds).Total();
}

FRTPhaseTime URTPlaybackLibrary::PhaseTime(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumActivations,
	int32 NumSequenceElements, float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds)
{
	// Il tempo di movimento e' lo stesso calcolo per tutte le fasi che muovono, Blast compreso: si scrive
	// una volta sola perche' due copie divergerebbero alla prima modifica di una delle due.
	const float MoveTime = (CellsPerSecond > 0.f)
		? (FMath::Max(0, MaxMoveSegments) / CellsPerSecond)
		: 0.f;
	// Il tempo delle attivazioni di Prep e Dash (#3549): mostrato, quindi incomprimibile come i colpi.
	const float ActivationTime = FMath::Max(0, NumActivations) * FMath::Max(0.f, AttackShowSeconds);

	FRTPhaseTime Out;

	switch (Phase)
	{
	case ERTMatchPhase::Dash:
		// Prima le attivazioni, poi le rotte: `RouteAlpha` parte dopo il loro tempo.
		Out.Shown = ActivationTime + MoveTime;
		break;

	case ERTMatchPhase::Move:
		// Tutto movimento: non c'e' attesa da togliere, e toglierla sarebbe accelerare i cilindri.
		Out.Shown = MoveTime;
		break;

	case ERTMatchPhase::Blast:
	{
		// 🔴 **La SEQUENZA, non il `Max` fra canali** (#3549, D5). I tre canali paralleli di #2828/#3278 sono
		// diventati una sequenza per intento, svelata un elemento per volta: la fase dura quanto la sequenza.
		// `Max(1, ...)`: un Blast di sola spinta si vede e non puo' durare zero.
		// ⏱️ *Fino a #3549 qui c'era il `Max` fra colpi, muri e impronte, rivelati in parallelo.*
		const float SequenceTime = FMath::Max(1, NumSequenceElements) * AttackShowSeconds;
		// `Max` con la spinta e non somma: i colpi si vedono MENTRE il bersaglio scivola, non dopo.
		//
		// 🔴 **Tutto `Shown`, zero `Slack`, e la prima stesura sbagliava qui.** Metteva in `Slack`
		// l'eccedenza `SequenceTime - MoveTime`, ragionando che fosse tempo «di lettura» e quindi
		// comprimibile. Non lo e': l'ordine di recupero di #1878 autorizza i beat delle fasi che NON
		// mostrano nulla, e questa mostra i colpi. Comprimerlo faceva due danni — la fase poteva durare
		// zero e i colpi uscivano tutti in un frame, e la spinta accelerava fino al rate base perche'
		// `Alpha` la misura su `PhaseDur`.
		Out.Shown = FMath::Max(SequenceTime, MoveTime);
		break;
	}

	case ERTMatchPhase::Prep:
		// Le attivazioni si mostrano; il beat di oggi resta, ed e' l'unica parte che il budget puo' togliere.
		Out.Shown = ActivationTime;
		Out.Slack = PhaseBeatSeconds;
		break;

	default:
		// Cleanup, Planning: un beat, e non c'e' niente da guardare mentre passa. E' l'unica attesa che il
		// budget puo' togliere.
		Out.Slack = PhaseBeatSeconds;
		break;
	}

	return Out;
}

float URTPlaybackLibrary::SlackScaleForBudget(float ShownSeconds, float SlackSeconds, float MaxSeconds)
{
	// Budget non dichiarato = nessun limite: niente da comprimere.
	if (MaxSeconds <= 0.f)
	{
		return 1.f;
	}

	// Nessuno slack da togliere. La risposta e' 1 e non 0 perche' «non c'e' niente da comprimere» non e'
	// «comprimi tutto»: moltiplicare zero per l'uno o per l'altro da' lo stesso tempo, ma il valore
	// restituito e' anche telemetria, e un 0 direbbe che il budget ha agito quando non aveva su cosa.
	if (SlackSeconds <= 0.f)
	{
		return 1.f;
	}

	// Gia' dentro: niente compressione.
	if (ShownSeconds + SlackSeconds <= MaxSeconds)
	{
		return 1.f;
	}

	// Quanto slack ci sta nello spazio che la locomozione lascia libera. Il clamp basso a zero e' il punto
	// in cui il budget diventa SOFT: sotto zero significherebbe togliere tempo al movimento, cioe'
	// accelerarlo, ed e' esattamente cio' che #1878 esclude.
	return FMath::Clamp((MaxSeconds - ShownSeconds) / SlackSeconds, 0.f, 1.f);
}

float URTPlaybackLibrary::EffectivePlaybackSpeed(float ViewerSpeed)
{
	// Non positivo = "non scelto": vale 1, cosi' la riproduzione resta definita anche su un campo azzerato
	// (variabile Blueprint, Memzero) invece di fermarsi.
	return (ViewerSpeed > 0.f) ? ViewerSpeed : 1.f;
}

float URTPlaybackLibrary::RouteAlpha(int32 RouteSegments, float PhaseElapsed, float CellsPerSecond)
{
	if (RouteSegments <= 0 || CellsPerSecond <= 0.f)
	{
		// Niente da attraversare, o movimento istantaneo: il percorso e' gia' concluso. Non e' il caso
		// degenere di una divisione per zero — e' la stessa convenzione di `PhaseDuration`.
		return 1.f;
	}
	return FMath::Clamp((PhaseElapsed * CellsPerSecond) / static_cast<float>(RouteSegments), 0.f, 1.f);
}

float URTPlaybackLibrary::DirectionYaw(const FVector& From, const FVector& To)
{
	const FVector Dir = To - From;
	if (Dir.SizeSquared2D() <= UE_KINDA_SMALL_NUMBER)
	{
		return 0.f; // nessuna direzione planare -> nessun orientamento
	}
	// Atan2(Y,X): +X=0, +Y=90, -X=+/-180, -Y=-90 (convenzione yaw UE). Z ignorata (facing planare).
	return FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
}

int32 URTPlaybackLibrary::MicroStepsInPath(const TArray<FVector>& Waypoints)
{
	// `Max(0, ...)` e non `Num()-1` nudo: su un array vuoto quello darebbe `-1`, e un conteggio negativo
	// propagherebbe un divisore assurdo dentro `AlphaAtMicroStep`.
	return FMath::Max(0, Waypoints.Num() - 1);
}

float URTPlaybackLibrary::AlphaAtMicroStep(int32 StepIndex, int32 StepCount)
{
	if (StepCount <= 0)
	{
		// Nessun segmento: la fase e' gia' conclusa. `1.f` e non `0.f`, altrimenti un percorso degenere
		// resterebbe in attesa di un avanzamento che non puo' avvenire.
		return 1.f;
	}
	return FMath::Clamp(static_cast<float>(StepIndex) / static_cast<float>(StepCount), 0.f, 1.f);
}

float URTPlaybackLibrary::PivotYaw(float FromYaw, float ToYaw, float Progress)
{
	// L'arco PIU' CORTO: da 170 a -170 sono venti gradi, non trecentoquaranta.
	const float Delta = FMath::FindDeltaAngleDegrees(FromYaw, ToYaw);
	return FRotator::NormalizeAxis(FromYaw + Delta * FMath::Clamp(Progress, 0.f, 1.f));
}

float URTPlaybackLibrary::StepYawAtAlpha(const TArray<FVector>& World, float Alpha, float EntryYaw, float TurnFraction)
{
	const int32 Segmenti = World.Num() - 1;
	if (Segmenti < 1)
	{
		return EntryYaw;
	}

	// Lo yaw del segmento `I`, o del primo con una direzione andando indietro; `EntryYaw` se nessuno ne ha.
	const auto YawFinoA = [&World, EntryYaw](int32 I)
	{
		for (int32 J = I; J >= 0; --J)
		{
			if ((World[J + 1] - World[J]).SizeSquared2D() > UE_KINDA_SMALL_NUMBER)
			{
				return DirectionYaw(World[J], World[J + 1]);
			}
		}
		return EntryYaw;
	};

	const float A = FMath::Clamp(Alpha, 0.f, 1.f);
	// 🔑 La cella si chiede a `MicroStepAtAlpha`, come fanno velo e rivelazione: su un confine esatto la mesh deve
	// stare sulla STESSA cella che leggono loro, non su quella prima per un arrotondamento.
	const int32 K = FMath::Min(MicroStepAtAlpha(A, Segmenti), Segmenti);
	if (K >= Segmenti)
	{
		return YawFinoA(Segmenti - 1);
	}
	const float Prima = (K == 0) ? EntryYaw : YawFinoA(K - 1);
	const float Dopo = YawFinoA(K);
	const float Frazione = FMath::Clamp(A * Segmenti - K, 0.f, 1.f);
	const float Giro = (TurnFraction > 0.f) ? FMath::Clamp(Frazione / TurnFraction, 0.f, 1.f) : 1.f;
	return PivotYaw(Prima, Dopo, Giro);
}

int32 URTPlaybackLibrary::MicroStepAtAlpha(float Alpha, int32 StepCount)
{
	if (StepCount <= 0)
	{
		return 0; // nessun segmento: non c'e' un micro-step da contare
	}

	// ⚠️ **`UE_KINDA_SMALL_NUMBER` si SOMMA, e il verso e' la parte che sbaglia facilmente.** `Alpha`
	// arriva da un'accumulazione in virgola mobile, e `1/3` vale `0.333333343`: senza tolleranza quel
	// valore cadrebbe appena SOTTO il proprio confine, e il floor lo assegnerebbe al segmento precedente.
	// ⛔ Sottrarla fa esattamente questo difetto su OGNI confine esatto, non solo su quelli inesatti:
	// misurato, `NextMicroStepBoundary(0.25f, 4)` restituiva `0.25` invece di `0.5`.
	//
	// 🔑 **Per `StepCount`, e non diviso per `1 / StepCount`**: e' l'inversa diretta di `AlphaAtMicroStep`,
	// e risparmia l'arrotondamento del reciproco. Fino a `#3458` `NextMicroStepBoundary` divideva per il
	// passo; la differenza fra le due forme sta sotto la tolleranza, e
	// `Playback.MicroStepAtAlphaLandsOnTheBoundaryAStepStopsAt` la percorre da 1 a 12 segmenti.
	return FMath::FloorToInt((FMath::Max(0.f, Alpha) + UE_KINDA_SMALL_NUMBER) * static_cast<float>(StepCount));
}

float URTPlaybackLibrary::NextMicroStepBoundary(float Alpha, int32 StepCount)
{
	if (StepCount <= 0)
	{
		return 1.f; // niente da attraversare
	}

	// 🔴 **`MicroStepAtAlpha + 1`, e il `+1` e' la regola**: si va al confine SUCCESSIVO anche quando
	// `Alpha` e' gia' esattamente su uno. Con un arrotondamento «al piu' vicino >=» premere `Step` due volte
	// su un boundary non farebbe nulla la seconda volta. La tolleranza che fa riconoscere quel confine e'
	// in `MicroStepAtAlpha`: senza, «il prossimo» sarebbe il confine su cui ci si trova gia', cioe' `Step`
	// non avanzerebbe.
	const int32 Prossimo = FMath::Max(0, MicroStepAtAlpha(Alpha, StepCount)) + 1;

	return AlphaAtMicroStep(Prossimo, StepCount);
}

int32 URTPlaybackLibrary::NextActionBoundary(const TArray<FRTResolvedEvent>& Timeline, int32 FromIndex)
{
	const int32 Fine = Timeline.Num();

	// 🔑 **L'atto in corso si cerca ALL'INDIETRO, e non e' un dettaglio.** Un `Defeated` o un danno
	// ambientale portano `NAME_None` — non li ha *fatti* nessuno — quindi leggere l'azione corrente dal
	// solo evento a `FromIndex` la perderebbe ogni volta che ci si ferma su uno di essi, e il colpo che
	// segue, pur essendo lo STESSO atto, sembrerebbe aprirne uno nuovo. Su
	// `Attack(A) · Defeated(None) · Attack(A)` la lettura ingenua fermerebbe `Next Action` due volte
	// dentro un colpo solo.
	//
	// ⚠️ `FromIndex` negativo significa «prima dell'inizio»: nessun atto in corso, e il primo evento con
	// un'azione e' gia' un confine. `Min(FromIndex, Fine - 1)` tiene la scansione dentro l'array anche
	// quando l'indice arriva oltre la fine, e su timeline vuota il ciclo non parte.
	FName Corrente = NAME_None;
	for (int32 i = FMath::Min(FromIndex, Fine - 1); i >= 0; --i)
	{
		if (!Timeline[i].ActionId.IsNone())
		{
			Corrente = Timeline[i].ActionId;
			break;
		}
	}

	for (int32 i = FMath::Max(0, FromIndex + 1); i < Fine; ++i)
	{
		// 🔑 **La regola NON si riscrive qui**: e' `IsActBoundary`, e da `#3292` ha un solo posto. Questa
		// funzione e' la sua vista su una timeline — decide COSA sia l'atto in corso (la scansione
		// all'indietro qui sopra) e delega il resto.
		if (IsActBoundary(Timeline[i], Corrente))
		{
			return i;
		}
	}

	// Nessun altro atto: la fine della timeline. E' la stessa scelta di `NextMicroStepBoundary`, che oltre
	// l'ultimo segmento porta a fine fase e non oltre.
	return Fine;
}

bool URTPlaybackLibrary::IsActBoundary(const FRTResolvedEvent& Event, FName CurrentAction)
{
	// 🔴 **`StructureHit` senza azione E' un confine, e su nessun altro tipo lo e'** — `#3281`, [D-437].
	// Su questo tipo `NAME_None` non significa *«nessuna azione dietro»*: significa **«piu' di uno l'ha
	// fatto»**, perche' il produttore nomina l'azione quando l'autore e' uno e tace solo sull'aggregato.
	// ⛔ Fermarsi ci sta: un muro che cade qualcuno l'ha fatto cadere, e l'evento porta chi.
	//
	// ⛔ **`ArcHit` resta fuori, deliberatamente**: porta `NAME_None` sempre (`#3280`), quindi qui
	// diventerebbe un confine a ogni arco colpito — e se sia giusto e' una decisione che nessuna issue ha
	// preso. Chi la prende aggiunga il tipo qui, con la sua ragione.
	if (Event.ActionId.IsNone())
	{
		return Event.Type == ERTResolvedEventType::StructureHit;
	}

	// ⚠️ **Piu' eventi con lo stesso `ActionId` sono UN atto**, ed e' la riga che risolve il caso
	// dell'impronta: impronta e colpi nascono dallo stesso intento, quindi portano la stessa azione e non
	// fanno fermare due volte. Stessa ragione per cui un'area su tre bersagli e' un atto solo.
	return Event.ActionId != CurrentAction;
}

namespace
{
	/** Il rango di un tipo DENTRO un atto: attivazione, impronte, muri, colpi (spec §2.4). -1 = non entra. */
	int32 RTRangoNellAtto(ERTResolvedEventType Type)
	{
		switch (Type)
		{
		case ERTResolvedEventType::AbilityActivated: return 0;
		case ERTResolvedEventType::AttackFootprint:  return 1;
		case ERTResolvedEventType::StructureHit:     return 2;
		case ERTResolvedEventType::Attack:           return 3;
		default:                                     return -1; // `ArcHit` compreso: #3293
		}
	}
}

TArray<FRTBlastSequenceElement> URTPlaybackLibrary::BuildBlastSequence(const TArray<FRTResolvedEvent>& Timeline,
	const TArray<FRTBlastSequenceElement>& Previous, int32 FrozenPrefix, int32 ViewerTeamId)
{
	TArray<FRTBlastSequenceElement> Out;

	// D-355: il prefisso gia' mostrato si riproduce VERBATIM. ⚠️ `TSet` solo per `Contains`.
	const int32 Congelati = FMath::Clamp(FrozenPrefix, 0, Previous.Num());
	TSet<int32> GiaInSequenza;
	for (int32 i = 0; i < Congelati; ++i)
	{
		Out.Add(Previous[i]);
		GiaInSequenza.Add(Previous[i].TimelineIndex);
	}

	struct FRTAttoInCostruzione
	{
		int32 Source = 0;
		FName ActionId;
		TArray<int32> Indici;
		int32 PrimaApparizione = INDEX_NONE;
		int32 IndiceAttivazione = INDEX_NONE; // l'attivazione VISIBILE del gruppo, se c'e'
		/** La chiave d'ordine fra gli atti (decisione (d)): l'attivazione se c'e', altrimenti la prima apparizione. */
		int32 Chiave() const { return IndiceAttivazione != INDEX_NONE ? IndiceAttivazione : PrimaApparizione; }
	};
	// Gli atti nascono nell'ordine di prima apparizione (la scansione va in avanti) e si RIORDINANO poi per
	// `Chiave()`: uno `StructureHit` emesso prima delle attivazioni non deve trascinare il suo intento davanti a
	// quelli con `IntentIndex` minore. ⚠️ Le chiavi sono indici di timeline distinti: l'ordine e' totale.
	TArray<FRTAttoInCostruzione> Atti;

	for (int32 i = 0; i < Timeline.Num(); ++i)
	{
		const FRTResolvedEvent& Ev = Timeline[i];
		if (Ev.Phase != ERTMatchPhase::Blast || RTRangoNellAtto(Ev.Type) < 0 || GiaInSequenza.Contains(i))
		{
			continue;
		}
		// D6: un'attivazione che chi guarda non ha il diritto di vedere non entra — tutto o niente.
		if (Ev.Type == ERTResolvedEventType::AbilityActivated && !Ev.SourceVerdict.AllowsTeam(ViewerTeamId))
		{
			continue;
		}

		int32 Atto = INDEX_NONE;
		if (!Ev.ActionId.IsNone()) // senza identita' = atto proprio, mai fuso (#3281)
		{
			Atto = Atti.IndexOfByPredicate([&Ev](const FRTAttoInCostruzione& A)
			{
				return !A.ActionId.IsNone() && A.Source == Ev.SourceStableUnitId && A.ActionId == Ev.ActionId;
			});
		}
		if (Atto == INDEX_NONE)
		{
			FRTAttoInCostruzione& Nuovo = Atti.AddDefaulted_GetRef();
			Nuovo.Source = Ev.SourceStableUnitId;
			Nuovo.ActionId = Ev.ActionId;
			Nuovo.PrimaApparizione = i;
			Atto = Atti.Num() - 1;
		}
		Atti[Atto].Indici.Add(i);
		if (Ev.Type == ERTResolvedEventType::AbilityActivated && Atti[Atto].IndiceAttivazione == INDEX_NONE)
		{
			Atti[Atto].IndiceAttivazione = i; // solo le VISIBILI arrivano qui: le altre sono uscite sopra
		}
	}

	// Decisione (d): l'ordine fra gli atti e' quello delle loro attivazioni.
	Atti.StableSort([](const FRTAttoInCostruzione& X, const FRTAttoInCostruzione& Y) { return X.Chiave() < Y.Chiave(); });

	for (FRTAttoInCostruzione& A : Atti)
	{
		// Ordine TOTALE (rango, poi indice): nessuna dipendenza dalla stabilita' dell'algoritmo.
		A.Indici.StableSort([&Timeline](int32 X, int32 Y)
		{
			const int32 RX = RTRangoNellAtto(Timeline[X].Type);
			const int32 RY = RTRangoNellAtto(Timeline[Y].Type);
			return (RX != RY) ? (RX < RY) : (X < Y);
		});
		for (const int32 Indice : A.Indici)
		{
			FRTBlastSequenceElement E;
			E.TimelineIndex = Indice;
			E.SourceStableUnitId = Timeline[Indice].SourceStableUnitId;
			E.ActionId = Timeline[Indice].ActionId;
			Out.Add(E);
		}
	}
	return Out;
}
