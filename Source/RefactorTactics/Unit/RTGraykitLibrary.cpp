#include "Unit/RTGraykitLibrary.h"

namespace
{
	/** Un ingresso avvelenato non deve propagarsi in una trasformazione. `NaN` e infiniti diventano `0`. */
	float Sane(float Value)
	{
		return FMath::IsFinite(Value) ? Value : 0.f;
	}

	/**
	 * Di quanto si sposta il PIVOT quando la parte ruota attorno a un anchor diverso dal centro.
	 *
	 * Un punto `P` ruotato attorno ad `A` va in `A + Q*(P-A)`. Il pivot e' `P = 0`, quindi finisce in
	 * `A - Q*A`: e' lo scarto che serve perche' un `Lean` con perno a `Ground` tenga davvero i piedi fermi
	 * invece di far affondare il cilindro.
	 */
	FVector PivotShift(const FQuat& Rotation, const FVector& Anchor)
	{
		return Anchor - Rotation.RotateVector(Anchor);
	}

	/** L'asse di una rotazione, con una ricaduta esplicita quando la direzione e' degenere. */
	FVector SafeAxis(const FVector& Direction, const FVector& Fallback)
	{
		const FVector Normalized = Direction.GetSafeNormal();
		return Normalized.IsNearlyZero() ? Fallback : Normalized;
	}
}

float URTGraykitLibrary::WindowAlpha(const FRTGraykitOperator& Op, float NormalizedTime)
{
	// ⛔ Durata non positiva (o non finita) = operatore inerte. Vedi il docstring del campo: un impulso di
	// durata nulla non e' rappresentabile a tempo campionato, e trattarlo come «sempre attivo» lo renderebbe
	// indistinguibile da una finestra piena.
	if (!FMath::IsFinite(Op.Duration) || Op.Duration <= 0.f)
	{
		return 0.f;
	}

	const float T = FMath::Clamp(Sane(NormalizedTime), 0.f, 1.f);
	const float Start = FMath::Clamp(Sane(Op.StartTime), 0.f, 1.f);
	return FMath::Clamp((T - Start) / Op.Duration, 0.f, 1.f);
}

float URTGraykitLibrary::ApplyEasing(ERTGraykitEasing Easing, float Alpha, float Cycles)
{
	const float T = FMath::Clamp(Sane(Alpha), 0.f, 1.f);

	switch (Easing)
	{
	case ERTGraykitEasing::EaseIn:
		return T * T;

	case ERTGraykitEasing::EaseOut:
		return 1.f - (1.f - T) * (1.f - T);

	case ERTGraykitEasing::EaseInOut:
		return (T < 0.5f) ? (2.f * T * T) : (1.f - 2.f * (1.f - T) * (1.f - T));

	case ERTGraykitEasing::PingPong:
	{
		const float C = FMath::Max(Sane(Cycles), 0.f);
		if (C <= 0.f)
		{
			return 0.f;
		}
		// La fase ciclica, cosi' che a fine finestra si torni esattamente a riposo: con `C` intero,
		// `Phase` e' intero e `Frac` e' zero.
		const float Phase = T * C;
		const float Frac = Phase - FMath::FloorToFloat(Phase);
		return FMath::Sin(PI * Frac);
	}

	case ERTGraykitEasing::Linear:
	default:
		return T;
	}
}

FVector URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor Anchor, float HalfHeight, float Radius)
{
	const float H = Sane(HalfHeight);
	const float R = Sane(Radius);

	// L'altezza della mano: poco sopra la meta' del busto. E' una frazione di `H` e non un numero fisso,
	// perche' un cilindro ridimensionato deve portarsi dietro le proprie mani.
	const float HandZ = H * 0.35f;

	switch (Anchor)
	{
	case ERTGraykitAnchor::Ground:    return FVector(0.f, 0.f, -H);
	case ERTGraykitAnchor::Top:       return FVector(0.f, 0.f, H);
	case ERTGraykitAnchor::Front:     return FVector(R, 0.f, 0.f);
	case ERTGraykitAnchor::Back:      return FVector(-R, 0.f, 0.f);
	case ERTGraykitAnchor::Left:      return FVector(0.f, -R, 0.f);
	case ERTGraykitAnchor::Right:     return FVector(0.f, R, 0.f);
	case ERTGraykitAnchor::LeftHand:  return FVector(0.f, -R, HandZ);
	case ERTGraykitAnchor::RightHand: return FVector(0.f, R, HandZ);
	case ERTGraykitAnchor::Center:
	default:                          return FVector::ZeroVector;
	}
}

TArray<ERTGraykitAnchor> URTGraykitLibrary::AllAnchors()
{
	return {
		ERTGraykitAnchor::Center,
		ERTGraykitAnchor::Ground,
		ERTGraykitAnchor::Top,
		ERTGraykitAnchor::Front,
		ERTGraykitAnchor::Back,
		ERTGraykitAnchor::Left,
		ERTGraykitAnchor::Right,
		ERTGraykitAnchor::LeftHand,
		ERTGraykitAnchor::RightHand
	};
}

TArray<ERTGraykitLocomotionStyle> URTGraykitLibrary::AllStyles()
{
	return {
		ERTGraykitLocomotionStyle::Normal,
		ERTGraykitLocomotionStyle::Run,
		ERTGraykitLocomotionStyle::Stealth,
		ERTGraykitLocomotionStyle::Reduced
	};
}

FRTGraykitPose URTGraykitLibrary::Evaluate(const FRTGraykitDescriptor& Descriptor, float NormalizedTime,
	float HalfHeight, float Radius)
{
	FRTGraykitPose Pose;
	const float T = FMath::Clamp(Sane(NormalizedTime), 0.f, 1.f);

	for (const FRTGraykitOperator& Op : Descriptor.Operators)
	{
		const float Raw = WindowAlpha(Op, T);
		const float Magnitude = Sane(Op.Magnitude);
		if (Magnitude == 0.f)
		{
			continue;
		}

		FRTGraykitPart& Part = Pose.PartFor(Op.Target);

		switch (Op.Op)
		{
		case ERTGraykitOperator::Translate:
		{
			const FVector Dir = Op.Direction.GetSafeNormal();
			Part.Offset += Dir * (Magnitude * ApplyEasing(Op.Easing, Raw, Op.Cycles));
			break;
		}

		case ERTGraykitOperator::Rotate:
		{
			// ⚠️ Le rotazioni si accumulano sommando gli EULER, non componendo quaternioni, ed e' cio' che
			// rende `Evaluate` indipendente dall'ordine (il docstring di `FRTGraykitDescriptor` lo promette
			// e `FRTGraykitOrderIndependenceTest` lo misura). La composizione di quaternioni non e'
			// commutativa: due descriptor con gli stessi operatori in ordine diverso darebbero pose diverse.
			const float Degrees = Magnitude * ApplyEasing(Op.Easing, Raw, Op.Cycles);
			const FVector Axis = SafeAxis(Op.Direction, FVector::UpVector);
			const FQuat Q(Axis, FMath::DegreesToRadians(Degrees));
			Part.Rotation += Q.Rotator();
			Part.Offset += PivotShift(Q, AnchorOffset(Op.Anchor, HalfHeight, Radius));
			break;
		}

		case ERTGraykitOperator::Lean:
		{
			// L'inclinazione e' una rotazione attorno all'asse ORIZZONTALE perpendicolare alla direzione di
			// marcia, con perno ai piedi. L'anchor dichiarato viene ignorato: un `Lean` con perno alla testa
			// non e' un'inclinazione, e' un rovesciamento.
			const float Degrees = Magnitude * ApplyEasing(Op.Easing, Raw, Op.Cycles);
			const FVector Dir = SafeAxis(Op.Direction, FVector::ForwardVector);
			const FVector Axis = SafeAxis(FVector::CrossProduct(FVector::UpVector, Dir), FVector::RightVector);
			const FQuat Q(Axis, FMath::DegreesToRadians(Degrees));
			Part.Rotation += Q.Rotator();
			Part.Offset += PivotShift(Q, AnchorOffset(ERTGraykitAnchor::Ground, HalfHeight, Radius));
			break;
		}

		case ERTGraykitOperator::Squash:
		{
			// Volume grossolanamente conservato: cio' che si perde in altezza si guadagna di traverso.
			// «Grossolanamente» e' letterale — e' una lettura a occhio, non un invariante fisico, e nessun
			// test lo verifica come tale.
			const float K = Magnitude * ApplyEasing(Op.Easing, Raw, Op.Cycles);
			Part.Scale *= FVector(1.f + K * 0.5f, 1.f + K * 0.5f, 1.f - K);
			break;
		}

		case ERTGraykitOperator::Stretch:
		{
			const float K = Magnitude * ApplyEasing(Op.Easing, Raw, Op.Cycles);
			Part.Scale *= FVector(1.f - K * 0.5f, 1.f - K * 0.5f, 1.f + K);
			break;
		}

		case ERTGraykitOperator::Pulse:
		{
			// 🔑 Oscillatore: la fase viene da `Raw` e `Cycles`, e l'easing NON si applica. Pesare un
			// oscillatore con un easing che a sua volta oscilla (`PingPong`) produrrebbe un battimento che
			// nessun autore ha chiesto e che nessuno saprebbe leggere in un descriptor.
			const float Cycle = FMath::Sin(2.f * PI * Raw * FMath::Max(Sane(Op.Cycles), 0.f));
			Part.Scale *= FVector(1.f + Magnitude * Cycle);
			break;
		}

		case ERTGraykitOperator::Bob:
		{
			// Stesso patto di `Pulse`: oscillatore, easing ignorato.
			const float Cycle = FMath::Sin(2.f * PI * Raw * FMath::Max(Sane(Op.Cycles), 0.f));
			Part.Offset.Z += Magnitude * Cycle;
			break;
		}

		default:
			// ⛔ Operatore che questa build non conosce: contributo ZERO, e gli altri continuano. Un
			// descriptor scritto da una build piu' nuova degrada invece di sparire — che e' il verso giusto
			// per la presentazione, dove un `ensure` produrrebbe uno spam per frame su un difetto di dato.
			break;
		}
	}

	return Pose;
}

FRTGraykitDescriptor URTGraykitLibrary::DescriptorForStyle(ERTGraykitLocomotionStyle Style)
{
	FRTGraykitDescriptor Out;

	// Un operatore configurato in una riga sola: gli altri campi restano ai default della struct.
	auto Make = [](ERTGraykitOperator Op, ERTGraykitPart Target, float Magnitude, float Start, float Duration,
		ERTGraykitEasing Easing, float Cycles, const FVector& Direction)
	{
		FRTGraykitOperator O;
		O.Op = Op;
		O.Target = Target;
		O.Magnitude = Magnitude;
		O.StartTime = Start;
		O.Duration = Duration;
		O.Easing = Easing;
		O.Cycles = Cycles;
		O.Direction = Direction;
		return O;
	};

	const FVector Fwd = FVector::ForwardVector;
	const FVector Right = FVector::RightVector;

	switch (Style)
	{
	case ERTGraykitLocomotionStyle::Run:
		Out.Label = TEXT("Locomotion.Run");
		// Inclinazione marcata, tenuta per tutta la corsa: e' la lettura piu' forte a distanza.
		Out.Operators.Add(Make(ERTGraykitOperator::Lean, ERTGraykitPart::Body, 18.f, 0.f, 0.25f, ERTGraykitEasing::EaseOut, 1.f, Fwd));
		// Allungamento direzionale: cio' che `Normal` non ha affatto, non «lo stesso piu' grande».
		Out.Operators.Add(Make(ERTGraykitOperator::Stretch, ERTGraykitPart::Body, 0.12f, 0.f, 0.3f, ERTGraykitEasing::EaseOut, 1.f, Fwd));
		Out.Operators.Add(Make(ERTGraykitOperator::Bob, ERTGraykitPart::Body, 9.f, 0.f, 1.f, ERTGraykitEasing::Linear, 4.f, Fwd));
		Out.Operators.Add(Make(ERTGraykitOperator::Rotate, ERTGraykitPart::LeftArm, 55.f, 0.f, 1.f, ERTGraykitEasing::PingPong, 4.f, Right));
		Out.Operators.Add(Make(ERTGraykitOperator::Rotate, ERTGraykitPart::RightArm, -55.f, 0.f, 1.f, ERTGraykitEasing::PingPong, 4.f, Right));
		// Assestamento aggressivo: schiaccia all'arrivo.
		Out.Operators.Add(Make(ERTGraykitOperator::Squash, ERTGraykitPart::Body, 0.16f, 0.85f, 0.15f, ERTGraykitEasing::PingPong, 1.f, Fwd));
		break;

	case ERTGraykitLocomotionStyle::Stealth:
		Out.Label = TEXT("Locomotion.Stealth");
		// Baricentro basso tenuto per tutta l'azione, oscillazione compressa.
		Out.Operators.Add(Make(ERTGraykitOperator::Squash, ERTGraykitPart::Body, 0.18f, 0.f, 0.2f, ERTGraykitEasing::EaseOut, 1.f, Fwd));
		Out.Operators.Add(Make(ERTGraykitOperator::Lean, ERTGraykitPart::Body, 7.f, 0.f, 0.2f, ERTGraykitEasing::EaseOut, 1.f, Fwd));
		Out.Operators.Add(Make(ERTGraykitOperator::Bob, ERTGraykitPart::Body, 2.f, 0.f, 1.f, ERTGraykitEasing::Linear, 3.f, Fwd));
		Out.Operators.Add(Make(ERTGraykitOperator::Rotate, ERTGraykitPart::LeftArm, 12.f, 0.f, 1.f, ERTGraykitEasing::PingPong, 3.f, Right));
		Out.Operators.Add(Make(ERTGraykitOperator::Rotate, ERTGraykitPart::RightArm, -12.f, 0.f, 1.f, ERTGraykitEasing::PingPong, 3.f, Right));
		break;

	case ERTGraykitLocomotionStyle::Reduced:
		Out.Label = TEXT("Locomotion.Reduced");
		// 🔑 L'ASIMMETRIA e' il segnale: i due bracci non si specchiano, e un passo pesa piu' dell'altro.
		// E' cio' che distingue «ferito» da «lento», che un semplice rallentamento non direbbe.
		Out.Operators.Add(Make(ERTGraykitOperator::Bob, ERTGraykitPart::Body, 5.f, 0.f, 1.f, ERTGraykitEasing::Linear, 2.f, Fwd));
		Out.Operators.Add(Make(ERTGraykitOperator::Lean, ERTGraykitPart::Body, 6.f, 0.f, 0.3f, ERTGraykitEasing::EaseOut, 1.f, Right));
		Out.Operators.Add(Make(ERTGraykitOperator::Rotate, ERTGraykitPart::LeftArm, 22.f, 0.f, 1.f, ERTGraykitEasing::PingPong, 2.f, Right));
		Out.Operators.Add(Make(ERTGraykitOperator::Rotate, ERTGraykitPart::RightArm, -6.f, 0.f, 1.f, ERTGraykitEasing::PingPong, 2.f, Right));
		break;

	case ERTGraykitLocomotionStyle::Normal:
	default:
		Out.Label = TEXT("Locomotion.Normal");
		Out.Operators.Add(Make(ERTGraykitOperator::Lean, ERTGraykitPart::Body, 6.f, 0.f, 0.3f, ERTGraykitEasing::EaseOut, 1.f, Fwd));
		Out.Operators.Add(Make(ERTGraykitOperator::Bob, ERTGraykitPart::Body, 4.f, 0.f, 1.f, ERTGraykitEasing::Linear, 3.f, Fwd));
		Out.Operators.Add(Make(ERTGraykitOperator::Rotate, ERTGraykitPart::LeftArm, 22.f, 0.f, 1.f, ERTGraykitEasing::PingPong, 3.f, Right));
		Out.Operators.Add(Make(ERTGraykitOperator::Rotate, ERTGraykitPart::RightArm, -22.f, 0.f, 1.f, ERTGraykitEasing::PingPong, 3.f, Right));
		// Assestamento contenuto.
		Out.Operators.Add(Make(ERTGraykitOperator::Squash, ERTGraykitPart::Body, 0.06f, 0.9f, 0.1f, ERTGraykitEasing::PingPong, 1.f, Fwd));
		break;
	}

	return Out;
}

float URTGraykitLibrary::PoseDistance(const FRTGraykitPose& A, const FRTGraykitPose& B)
{
	float Total = 0.f;

	const ERTGraykitPart Parts[] = { ERTGraykitPart::Body, ERTGraykitPart::LeftArm, ERTGraykitPart::RightArm };
	for (ERTGraykitPart Which : Parts)
	{
		const FRTGraykitPart& PA = A.PartFor(Which);
		const FRTGraykitPart& PB = B.PartFor(Which);

		Total += FVector::Dist(PA.Offset, PB.Offset);
		Total += FMath::Abs(PA.Rotation.Pitch - PB.Rotation.Pitch)
			+ FMath::Abs(PA.Rotation.Yaw - PB.Rotation.Yaw)
			+ FMath::Abs(PA.Rotation.Roll - PB.Rotation.Roll);
		Total += FVector::Dist(PA.Scale, PB.Scale) * 100.f;
	}

	return Total;
}
