#include "Misc/AutomationTest.h"
#include "Unit/RTGraykitLibrary.h"
#include "Unit/RTGraykitTypes.h"
#include "Unit/RTUnit.h"
#include "Map/RTCellId.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Mondo per i test che hanno bisogno di un'unita' vera (nomi distinti per file: unity build). */
	UWorld* MakeGraykitWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/ false);
		if (World && GEngine)
		{
			FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
			Ctx.SetCurrentWorld(World);
		}
		return World;
	}

	FRTGraykitOperator MakeOp(ERTGraykitOperator Op, ERTGraykitPart Target, float Magnitude)
	{
		FRTGraykitOperator O;
		O.Op = Op;
		O.Target = Target;
		O.Magnitude = Magnitude;
		return O;
	}
}

// ---------------------------------------------------------------------------------------------------------
// 1. Determinismo: la stessa coppia (descriptor, tempo) da' la stessa posa, e arrivarci a salti o un passo
//    alla volta non fa differenza. E' la proprieta' che rende la posa utilizzabile in un replay che seeka.
// ---------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGraykitDeterminismTest,
	"RefactorTactics.Graykit.Determinismo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGraykitDeterminismTest::RunTest(const FString& Parameters)
{
	const FRTGraykitDescriptor Run = URTGraykitLibrary::DescriptorForStyle(ERTGraykitLocomotionStyle::Run);

	// (a) Ripetibilita' secca.
	for (int32 Step = 0; Step <= 20; ++Step)
	{
		const float T = static_cast<float>(Step) / 20.f;
		const FRTGraykitPose First = URTGraykitLibrary::Evaluate(Run, T);
		const FRTGraykitPose Second = URTGraykitLibrary::Evaluate(Run, T);
		TestTrue(FString::Printf(TEXT("a T=%.2f due valutazioni coincidono"), T),
			URTGraykitLibrary::PoseDistance(First, Second) == 0.f);
	}

	// (b) 🔑 Seek: valutare direttamente 0,73 da' la stessa posa che valutarlo dopo aver percorso la scala.
	// E' cio' che un'animazione a stato incrementale NON garantisce, ed e' la ragione per cui `Evaluate` e'
	// pura.
	FRTGraykitPose Walked;
	for (int32 Step = 0; Step <= 73; ++Step)
	{
		Walked = URTGraykitLibrary::Evaluate(Run, static_cast<float>(Step) / 100.f);
	}
	const FRTGraykitPose Sought = URTGraykitLibrary::Evaluate(Run, 0.73f);
	TestTrue(TEXT("il seek a 0,73 coincide col percorso passo-passo"),
		URTGraykitLibrary::PoseDistance(Walked, Sought) == 0.f);

	// (c) Ingressi fuori scala e avvelenati non producono pose non finite.
	const FRTGraykitPose Over = URTGraykitLibrary::Evaluate(Run, 1.4f);
	const FRTGraykitPose End = URTGraykitLibrary::Evaluate(Run, 1.f);
	TestTrue(TEXT("oltre 1 si ottiene la posa finale, non un'estrapolazione"),
		URTGraykitLibrary::PoseDistance(Over, End) == 0.f);

	const FRTGraykitPose Poisoned = URTGraykitLibrary::Evaluate(Run, FMath::Sqrt(-1.f));
	TestTrue(TEXT("un NaN in ingresso non produce un offset non finito"),
		Poisoned.Body.Offset.ContainsNaN() == false);
	TestTrue(TEXT("un NaN in ingresso non produce una scala non finita"),
		Poisoned.Body.Scale.ContainsNaN() == false);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 2. Composizione: l'ordine degli operatori non conta, ed e' cio' che permette di concatenare due descriptor
//    senza chiedersi chi viene prima.
// ---------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGraykitOrderIndependenceTest,
	"RefactorTactics.Graykit.ComposizioneIndipendenteDallOrdine", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGraykitOrderIndependenceTest::RunTest(const FString& Parameters)
{
	FRTGraykitDescriptor Forward;
	Forward.Operators.Add(MakeOp(ERTGraykitOperator::Translate, ERTGraykitPart::Body, 30.f));
	Forward.Operators.Add(MakeOp(ERTGraykitOperator::Bob, ERTGraykitPart::Body, 12.f));
	Forward.Operators.Add(MakeOp(ERTGraykitOperator::Squash, ERTGraykitPart::Body, 0.2f));
	Forward.Operators.Add(MakeOp(ERTGraykitOperator::Rotate, ERTGraykitPart::LeftArm, 40.f));

	FRTGraykitDescriptor Reversed;
	for (int32 i = Forward.Operators.Num() - 1; i >= 0; --i)
	{
		Reversed.Operators.Add(Forward.Operators[i]);
	}

	for (int32 Step = 0; Step <= 10; ++Step)
	{
		const float T = static_cast<float>(Step) / 10.f;
		const float Distance = URTGraykitLibrary::PoseDistance(
			URTGraykitLibrary::Evaluate(Forward, T),
			URTGraykitLibrary::Evaluate(Reversed, T));
		TestTrue(FString::Printf(TEXT("a T=%.1f l'ordine invertito da' la stessa posa (scarto %.6f)"), T, Distance),
			Distance < KINDA_SMALL_NUMBER);
	}

	// Due operatori sulla stessa parte si sommano invece di sovrascriversi.
	FRTGraykitDescriptor Twice;
	Twice.Operators.Add(MakeOp(ERTGraykitOperator::Translate, ERTGraykitPart::Body, 10.f));
	Twice.Operators.Add(MakeOp(ERTGraykitOperator::Translate, ERTGraykitPart::Body, 25.f));
	const FRTGraykitPose Summed = URTGraykitLibrary::Evaluate(Twice, 1.f);
	TestTrue(TEXT("due Translate si sommano (35 cm lungo il forward)"),
		FMath::IsNearlyEqual(Summed.Body.Offset.X, 35.f, 0.01f));

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 3. Le finestre temporali: un operatore fuori finestra non contribuisce, e uno di durata nulla e' inerte.
// ---------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGraykitWindowTest,
	"RefactorTactics.Graykit.FinestraTemporale", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGraykitWindowTest::RunTest(const FString& Parameters)
{
	FRTGraykitOperator Late;
	Late.Op = ERTGraykitOperator::Translate;
	Late.Magnitude = 50.f;
	Late.StartTime = 0.5f;
	Late.Duration = 0.5f;

	TestEqual(TEXT("prima dell'inizio la finestra vale 0"), URTGraykitLibrary::WindowAlpha(Late, 0.2f), 0.f);
	TestEqual(TEXT("a meta' finestra vale 0,5"), URTGraykitLibrary::WindowAlpha(Late, 0.75f), 0.5f);
	TestEqual(TEXT("a fine finestra vale 1"), URTGraykitLibrary::WindowAlpha(Late, 1.f), 1.f);

	// ⛔ Durata nulla = inerte, non permanente. Il criterio e' scritto nel docstring del campo.
	FRTGraykitOperator Zero = Late;
	Zero.Duration = 0.f;
	TestEqual(TEXT("durata zero e' inerte a inizio azione"), URTGraykitLibrary::WindowAlpha(Zero, 0.f), 0.f);
	TestEqual(TEXT("durata zero resta inerte a fine azione"), URTGraykitLibrary::WindowAlpha(Zero, 1.f), 0.f);

	// Un operatore concluso TIENE il proprio valore finale: e' cio' che serve a un assestamento.
	FRTGraykitDescriptor Early;
	FRTGraykitOperator Settle = Late;
	Settle.StartTime = 0.f;
	Settle.Duration = 0.2f;
	Early.Operators.Add(Settle);
	const FRTGraykitPose AtEnd = URTGraykitLibrary::Evaluate(Early, 1.f);
	TestTrue(TEXT("un operatore concluso tiene il valore finale invece di riavvolgersi"),
		FMath::IsNearlyEqual(AtEnd.Body.Offset.X, 50.f, 0.01f));

	// `PingPong` invece torna a riposo da solo a fine finestra.
	TestTrue(TEXT("PingPong torna a zero a fine finestra"),
		FMath::IsNearlyZero(URTGraykitLibrary::ApplyEasing(ERTGraykitEasing::PingPong, 1.f, 1.f), 0.001f));
	TestTrue(TEXT("PingPong culmina a meta' finestra"),
		FMath::IsNearlyEqual(URTGraykitLibrary::ApplyEasing(ERTGraykitEasing::PingPong, 0.5f, 1.f), 1.f, 0.001f));

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 4. Gli anchor: risolti per NOME dalle dimensioni, e sorveglianza sulla costante duplicata da `ARTUnit`.
// ---------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGraykitAnchorTest,
	"RefactorTactics.Graykit.Anchor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGraykitAnchorTest::RunTest(const FString& Parameters)
{
	// 🔴 La sorveglianza sulla duplicazione: `URTGraykitLibrary::DefaultHalfHeight` esiste per non includere
	// `RTUnit.h` dalla libreria. Se le due costanti divergessero, gli anchor cadrebbero fuori dal cilindro
	// senza che nulla lo dicesse. Questo test e' l'unica cosa che lo impedisce.
	TestEqual(TEXT("DefaultHalfHeight non e' divergito da ARTUnit::UnitHalfHeight"),
		URTGraykitLibrary::DefaultHalfHeight, ARTUnit::UnitHalfHeight);

	TestEqual(TEXT("Center e' l'origine"),
		URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor::Center), FVector::ZeroVector);
	TestEqual(TEXT("Ground sta sotto di mezza altezza"),
		URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor::Ground, 90.f, 45.f), FVector(0.f, 0.f, -90.f));
	TestEqual(TEXT("Top sta sopra di mezza altezza"),
		URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor::Top, 90.f, 45.f), FVector(0.f, 0.f, 90.f));
	TestEqual(TEXT("Front e' lungo il forward"),
		URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor::Front, 90.f, 45.f), FVector(45.f, 0.f, 0.f));
	// ⚠️ Unreal e' left-handed: la sinistra dell'attore e' -Y.
	TestTrue(TEXT("Left e' a -Y e Right a +Y"),
		URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor::Left, 90.f, 45.f).Y < 0.f &&
		URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor::Right, 90.f, 45.f).Y > 0.f);

	// Gli anchor sono NOMI: raddoppiare le dimensioni raddoppia gli offset, senza toccare i descriptor.
	const FVector Small = URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor::Ground, 90.f, 45.f);
	const FVector Large = URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor::Ground, 180.f, 90.f);
	TestTrue(TEXT("un cilindro doppio porta i propri anchor al doppio"), Large.Z == Small.Z * 2.f);

	// Le mani stanno sui fianchi, non nel centro geometrico.
	const FVector LeftHand = URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor::LeftHand, 90.f, 45.f);
	TestTrue(TEXT("LeftHand e' fuori dall'asse e sopra il centro"), LeftHand.Y < 0.f && LeftHand.Z > 0.f);

	TestEqual(TEXT("AllAnchors enumera i nove anchor dichiarati"), URTGraykitLibrary::AllAnchors().Num(), 9);

	// 🔑 Il perno funziona: un Lean tiene i piedi fermi. Il punto `Ground`, dopo la rotazione, non si muove.
	FRTGraykitDescriptor Leaning;
	FRTGraykitOperator Op;
	Op.Op = ERTGraykitOperator::Lean;
	Op.Magnitude = 20.f;
	Leaning.Operators.Add(Op);

	const FRTGraykitPose Pose = URTGraykitLibrary::Evaluate(Leaning, 1.f);
	const FVector GroundAnchor = URTGraykitLibrary::AnchorOffset(ERTGraykitAnchor::Ground, 90.f, 45.f);
	const FVector GroundAfter = Pose.Body.Offset + Pose.Body.Rotation.RotateVector(GroundAnchor);
	TestTrue(FString::Printf(TEXT("il Lean tiene i piedi fermi (scarto %.4f cm)"),
		FVector::Dist(GroundAfter, GroundAnchor)),
		FVector::Dist(GroundAfter, GroundAnchor) < 0.5f);

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 5. GATE GK-03 — Move e Run usano lo stesso sistema, hanno descriptor diversi, e Run non e' Move scalato.
// ---------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGraykitMoveRunDistinctTest,
	"RefactorTactics.Graykit.MoveNonEUgualeARun", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGraykitMoveRunDistinctTest::RunTest(const FString& Parameters)
{
	const FRTGraykitDescriptor Normal = URTGraykitLibrary::DescriptorForStyle(ERTGraykitLocomotionStyle::Normal);
	const FRTGraykitDescriptor Run = URTGraykitLibrary::DescriptorForStyle(ERTGraykitLocomotionStyle::Run);

	TestTrue(TEXT("entrambi gli stili producono operatori"),
		Normal.Operators.Num() > 0 && Run.Operators.Num() > 0);

	// 🔴 Il criterio del mandato: «non considerare Run semplicemente un Move piu' veloce». Se lo fosse, i due
	// descriptor avrebbero gli STESSI tipi di operatore con magnitudini diverse. La misura e' sull'insieme
	// dei tipi, non sulle pose: due pose diverse si otterrebbero anche solo scalando.
	TSet<ERTGraykitOperator> NormalKinds;
	for (const FRTGraykitOperator& O : Normal.Operators) { NormalKinds.Add(O.Op); }
	TSet<ERTGraykitOperator> RunKinds;
	for (const FRTGraykitOperator& O : Run.Operators) { RunKinds.Add(O.Op); }

	TestTrue(TEXT("Run porta almeno un tipo di operatore che Normal non ha"),
		RunKinds.Difference(NormalKinds).Num() > 0);
	TestTrue(TEXT("e il tipo in piu' e' lo Stretch direzionale"),
		RunKinds.Contains(ERTGraykitOperator::Stretch) && !NormalKinds.Contains(ERTGraykitOperator::Stretch));

	// Le pose differiscono per tutta l'azione, non solo in un istante fortunato.
	int32 DistinctSamples = 0;
	for (int32 Step = 1; Step <= 10; ++Step)
	{
		const float T = static_cast<float>(Step) / 10.f;
		if (URTGraykitLibrary::PoseDistance(
			URTGraykitLibrary::Evaluate(Normal, T),
			URTGraykitLibrary::Evaluate(Run, T)) > 1.f)
		{
			++DistinctSamples;
		}
	}
	TestTrue(FString::Printf(TEXT("le due pose differiscono in %d campioni su 10"), DistinctSamples),
		DistinctSamples >= 9);

	// Run si inclina piu' di Normal: e' la lettura che regge a distanza, senza HUD.
	const float NormalLean = FMath::Abs(URTGraykitLibrary::Evaluate(Normal, 0.5f).Body.Rotation.Pitch);
	const float RunLean = FMath::Abs(URTGraykitLibrary::Evaluate(Run, 0.5f).Body.Rotation.Pitch);
	TestTrue(FString::Printf(TEXT("Run si inclina piu' di Normal (%.1f contro %.1f gradi)"), RunLean, NormalLean),
		RunLean > NormalLean);

	// I quattro stili sono quattro descriptor distinti, non due piu' due alias.
	TestEqual(TEXT("gli stili dichiarati sono quattro"), URTGraykitLibrary::AllStyles().Num(), 4);
	TSet<FString> Labels;
	for (ERTGraykitLocomotionStyle Style : URTGraykitLibrary::AllStyles())
	{
		Labels.Add(URTGraykitLibrary::DescriptorForStyle(Style).Label);
	}
	TestEqual(TEXT("i quattro stili portano quattro etichette distinte"), Labels.Num(), 4);

	// ⚠️ `Reduced` e' ASIMMETRICO: e' il segnale che lo distingue da «lento».
	const FRTGraykitPose Reduced = URTGraykitLibrary::Evaluate(
		URTGraykitLibrary::DescriptorForStyle(ERTGraykitLocomotionStyle::Reduced), 0.25f);
	TestTrue(TEXT("in Reduced i due bracci non si specchiano"),
		!FMath::IsNearlyEqual(FMath::Abs(Reduced.LeftArm.Rotation.Pitch),
			FMath::Abs(Reduced.RightArm.Rotation.Pitch), 0.5f));

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 6. Fallback: un operatore che questa build non conosce contribuisce zero e non azzera gli altri.
// ---------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGraykitUnknownOperatorTest,
	"RefactorTactics.Graykit.OperatoreSconosciuto", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGraykitUnknownOperatorTest::RunTest(const FString& Parameters)
{
	// Un valore fuori dall'enum: e' cio' che arriva da un descriptor scritto da una build piu' nuova.
	FRTGraykitOperator Alien;
	Alien.Op = static_cast<ERTGraykitOperator>(200);
	Alien.Magnitude = 999.f;

	FRTGraykitDescriptor Mixed;
	Mixed.Operators.Add(Alien);
	Mixed.Operators.Add(MakeOp(ERTGraykitOperator::Translate, ERTGraykitPart::Body, 20.f));

	const FRTGraykitPose Pose = URTGraykitLibrary::Evaluate(Mixed, 1.f);
	TestTrue(TEXT("l'operatore noto e' stato applicato lo stesso"),
		FMath::IsNearlyEqual(Pose.Body.Offset.X, 20.f, 0.01f));
	TestTrue(TEXT("lo sconosciuto non ha aggiunto nulla"),
		FMath::IsNearlyZero(Pose.Body.Offset.Z, 0.01f));

	// Un descriptor vuoto produce la posa di riposo, non una posa degenere.
	const FRTGraykitDescriptor Empty;
	const FRTGraykitPose Rest = URTGraykitLibrary::Evaluate(Empty, 0.5f);
	TestEqual(TEXT("un descriptor vuoto lascia l'offset a zero"), Rest.Body.Offset, FVector::ZeroVector);
	TestEqual(TEXT("un descriptor vuoto lascia la scala a uno"), Rest.Body.Scale, FVector::OneVector);
	TestEqual(TEXT("un descriptor vuoto lascia i bracci a riposo"), Rest.LeftArm.Scale, FVector::OneVector);

	// Direzione degenere: nessuna divisione per zero, nessun NaN.
	FRTGraykitOperator Degenerate = MakeOp(ERTGraykitOperator::Rotate, ERTGraykitPart::Body, 45.f);
	Degenerate.Direction = FVector::ZeroVector;
	FRTGraykitDescriptor Degen;
	Degen.Operators.Add(Degenerate);
	const FRTGraykitPose DegenPose = URTGraykitLibrary::Evaluate(Degen, 1.f);
	TestFalse(TEXT("una direzione nulla non produce una rotazione NaN"), DegenPose.Body.Rotation.ContainsNaN());

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// 7. 🔴 L'invariante che conta: valutare e APPLICARE una posa non tocca il gameplay. La cella autorevole,
//    l'occupazione e la collisione restano quelle di prima.
// ---------------------------------------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGraykitNoGameplayMutationTest,
	"RefactorTactics.Graykit.NonToccaIlGameplay", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTGraykitNoGameplayMutationTest::RunTest(const FString& Parameters)
{
	UWorld* World = MakeGraykitWorld();
	if (World == nullptr)
	{
		AddError(TEXT("mondo di test non creato"));
		return false;
	}

	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	if (Unit == nullptr)
	{
		AddError(TEXT("unita' non creata"));
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}

	const FRTCellId CellBefore = Unit->Cell;
	const bool bCollisionBefore = Unit->GetActorEnableCollision();

	// Si valuta la locomozione piu' aggressiva a venti istanti diversi, e la si APPLICA come farebbe un
	// consumatore: offset e rotazione sul componente visivo, mai sull'attore.
	const FRTGraykitDescriptor Run = URTGraykitLibrary::DescriptorForStyle(ERTGraykitLocomotionStyle::Run);
	for (int32 Step = 0; Step <= 20; ++Step)
	{
		const FRTGraykitPose Pose = URTGraykitLibrary::Evaluate(Run, static_cast<float>(Step) / 20.f);
		if (UStaticMeshComponent* Body = Unit->FindComponentByClass<UStaticMeshComponent>())
		{
			Body->SetRelativeLocation(Pose.Body.Offset);
			Body->SetRelativeRotation(Pose.Body.Rotation);
			Body->SetRelativeScale3D(Pose.Body.Scale);
		}
	}

	TestEqual(TEXT("la cella autorevole non e' cambiata (X)"), Unit->Cell.X, CellBefore.X);
	TestEqual(TEXT("la cella autorevole non e' cambiata (Y)"), Unit->Cell.Y, CellBefore.Y);
	TestEqual(TEXT("la cella autorevole non e' cambiata (Layer)"), Unit->Cell.Layer, CellBefore.Layer);
	TestEqual(TEXT("la collisione dell'attore non e' cambiata"), Unit->GetActorEnableCollision(), bCollisionBefore);

	// ⛔ E la garanzia strutturale, che vale piu' della misura sopra: nessuna firma di `URTGraykitLibrary`
	// accetta un `AActor`. La posa non ha un canale per toccare il gameplay, quindi non serve fidarsi del
	// fatto che non lo faccia. Questo test misura che il CONSUMATORE tipico non lo faccia per conto suo.

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
