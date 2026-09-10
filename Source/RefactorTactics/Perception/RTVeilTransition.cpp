#include "Perception/RTVeilTransition.h"

float URTVeilTransitionLibrary::Advance(float Current, float Target, float DeltaSeconds,
	const FRTVeilTransitionParams& Params)
{
	// LA PAUSA, non una guardia difensiva: un passo nullo ferma la dissolvenza dov'e'. Scritto come
	// `!(x > 0)` e non come `x <= 0` perche' cosi' anche un NaN cade qui — un NaN propagato nel valore
	// disegnato spegnerebbe una cella e nessun log direbbe perche'.
	if (!(DeltaSeconds > 0.f))
	{
		return Current;
	}

	const float Delta = Target - Current;

	// Il VERSO sceglie la costante: si apre in fretta, si chiude piano. `Delta == 0` finisce nel ramo
	// reveal e non ha importanza — lo snap qui sotto lo chiude prima che la costante venga usata.
	const float Tau = (Delta >= 0.f) ? Params.RevealSeconds : Params.HideSeconds;

	// Il filtro SPENTO (`FRTVeilTransitionParams::Instant`): ogni passo arriva. E' il comportamento che il
	// velo aveva prima di `#2874`, e serve come ramo di confronto — non e' un ingresso da rifiutare.
	if (!(Tau > 0.f))
	{
		return Target;
	}

	// ⚠️ La soglia si legge una volta e si difende dal negativo: un `SnapEpsilon` sotto zero renderebbe
	// entrambi i confronti sempre falsi, e la transizione non finirebbe mai.
	const float Epsilon = FMath::Max(Params.SnapEpsilon, 0.f);

	// Gia' arrivati: si risponde il target ESATTO, cosi' che «converso» sia un'uguaglianza e non una
	// distanza. E' anche il caso piu' frequente di tutti — a regime la board e' ferma.
	if (FMath::Abs(Delta) <= Epsilon)
	{
		return Target;
	}

	// La forma dichiarata nell'header. `Lerp(C, T, 1 - e)` e' algebricamente `T + (C - T) * e`: la si scrive
	// cosi' perche' e' la forma in cui il prototipo l'ha specificata, e le due sono lo stesso numero.
	const float Alpha = 1.f - FMath::Exp(-DeltaSeconds / Tau);
	const float Next = FMath::Lerp(Current, Target, Alpha);

	// 🔴 Lo snap DOPO il passo, ed e' quello che conta davvero: senza, l'ultimo tratto non si chiude mai e
	// la cella resta «in transizione» per sempre, pagando una riscrittura per frame che nessuno vede.
	if (FMath::Abs(Target - Next) <= Epsilon)
	{
		return Target;
	}
	return Next;
}

int32 URTVeilTransitionLibrary::AdvanceAll(TArray<float>& Display, const TArray<float>& Targets,
	float DeltaSeconds, const FRTVeilTransitionParams& Params)
{
	// Fail-closed, e con un valore PROPRIO: `0` direbbe «tutto converso» e farebbe smettere di aggiornare
	// esattamente il chiamante che ha un ingresso rotto. `Display` non si tocca: mezzo aggiornamento su una
	// board disallineata sarebbe peggio di nessuno.
	if (Display.Num() != Targets.Num())
	{
		return INDEX_NONE;
	}

	int32 InMovimento = 0;
	for (int32 I = 0; I < Display.Num(); ++I)
	{
		const float Next = Advance(Display[I], Targets[I], DeltaSeconds, Params);
		Display[I] = Next;

		// L'uguaglianza ESATTA, e si puo' perche' `Advance` restituisce il target senza passare dal `Lerp`
		// quando la transizione e' finita. Un confronto per distanza qui duplicherebbe `SnapEpsilon` in un
		// secondo posto, e le due soglie divergerebbero al primo che ne cambia una sola.
		if (Next != Targets[I])
		{
			++InMovimento;
		}
	}
	return InMovimento;
}
