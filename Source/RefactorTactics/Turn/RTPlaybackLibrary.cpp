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

float URTPlaybackLibrary::PhaseDuration(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumAttacks,
	float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds)
{
	// Una riga: la formula sta in `PhaseTime`, e il totale e' la somma dei suoi due termini. Non c'e' un
	// secondo calcolo da tenere allineato.
	return PhaseTime(Phase, MaxMoveSegments, NumAttacks, CellsPerSecond, AttackShowSeconds,
		PhaseBeatSeconds).Total();
}

FRTPhaseTime URTPlaybackLibrary::PhaseTime(ERTMatchPhase Phase, int32 MaxMoveSegments, int32 NumAttacks,
	float CellsPerSecond, float AttackShowSeconds, float PhaseBeatSeconds)
{
	// Il tempo di movimento e' lo stesso calcolo per tutte le fasi che muovono, Blast compreso: si scrive
	// una volta sola perche' due copie divergerebbero alla prima modifica di una delle due.
	const float MoveTime = (CellsPerSecond > 0.f)
		? (FMath::Max(0, MaxMoveSegments) / CellsPerSecond)
		: 0.f;

	FRTPhaseTime Out;

	switch (Phase)
	{
	case ERTMatchPhase::Dash:
	case ERTMatchPhase::Move:
		// Tutto movimento: non c'e' attesa da togliere, e toglierla sarebbe accelerare i cilindri.
		Out.Shown = MoveTime;
		break;

	case ERTMatchPhase::Blast:
	{
		// `Max(1, ...)`: un Blast di sola spinta non ha colpi, e una fase che si vede non puo' durare zero.
		const float AttackTime = FMath::Max(1, NumAttacks) * AttackShowSeconds;
		// `Max` e non somma: i colpi si vedono MENTRE il bersaglio scivola, non dopo.
		//
		// 🔴 **Tutto `Shown`, zero `Slack`, e la prima stesura sbagliava qui.** Metteva in `Slack`
		// l'eccedenza `AttackTime - MoveTime`, ragionando che fosse tempo «di lettura» e quindi
		// comprimibile. Non lo e': l'ordine di recupero di #1878 autorizza i beat delle fasi che NON
		// mostrano nulla, e questa mostra i colpi. Comprimerlo faceva due danni — la fase poteva durare
		// zero e i colpi uscivano tutti in un frame, e la spinta accelerava fino al rate base perche'
		// `Alpha` la misura su `PhaseDur`.
		Out.Shown = FMath::Max(AttackTime, MoveTime);
		break;
	}

	default:
		// Prep, Cleanup, Planning: un beat, e non c'e' niente da guardare mentre passa. E' l'unica attesa
		// che il budget puo' togliere.
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

float URTPlaybackLibrary::NextMicroStepBoundary(float Alpha, int32 StepCount)
{
	if (StepCount <= 0)
	{
		return 1.f; // niente da attraversare
	}

	const float Passo = 1.f / static_cast<float>(StepCount);

	// 🔴 **`FloorToInt(Alpha/Passo) + 1`, e il `+1` e' la regola**: si va al confine SUCCESSIVO anche
	// quando `Alpha` e' gia' esattamente su uno. Con un arrotondamento «al piu' vicino >=» premere `Step`
	// due volte su un boundary non farebbe nulla la seconda volta.
	//
	// ⚠️ **`UE_KINDA_SMALL_NUMBER` si SOMMA, e il verso e' la parte che sbaglia facilmente.** `Alpha`
	// arriva da un'accumulazione in virgola mobile, e `1/3` vale `0.333333343`: senza tolleranza quel
	// valore cadrebbe appena SOTTO il proprio confine, il floor lo assegnerebbe al segmento precedente, e
	// «il prossimo» sarebbe il confine su cui ci si trova gia' — cioe' `Step` non avanzerebbe.
	// ⛔ Sottrarla fa esattamente questo difetto su OGNI confine esatto, non solo su quelli inesatti:
	// misurato, `NextMicroStepBoundary(0.25f, 4)` restituiva `0.25` invece di `0.5`.
	const int32 Corrente = FMath::FloorToInt((FMath::Max(0.f, Alpha) + UE_KINDA_SMALL_NUMBER) / Passo);
	const int32 Prossimo = FMath::Max(0, Corrente) + 1;

	return AlphaAtMicroStep(Prossimo, StepCount);
}
