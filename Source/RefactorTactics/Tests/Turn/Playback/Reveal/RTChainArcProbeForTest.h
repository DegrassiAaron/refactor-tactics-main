#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SplineComponent.h"
#include "RTChainArcProbeForTest.generated.h"

/**
 * Un attore con una `USplineComponent` come radice, solo per i test della scarica a catena ([D-621], #4136).
 *
 * Fa le veci di `BP_Gadget_LinearDischarge_Arc`: quel Blueprint sta sotto `Content/`, che git ignora, e in un worktree o
 * su un clone senza l'asset non si carica. Qui conta una cosa sola — che l'attore abbia una spline e nasca nel punto in
 * cui lo si spawna — cosi' un test puo' misurare il ramo felice di `URTPlaybackLibrary::SpawnChainArcActor` senza l'asset.
 *
 * 🔑 **La spline e' la radice, e non e' un dettaglio**: la trasformata dell'attore e' quella della spline, quindi un attore
 * spawnato lontano dall'origine ha una spline con la trasformata traslata. E' li' che `Local` e `World` si separano, e
 * con una spline non registrata e con trasformata identita' non si separerebbero mai.
 */
UCLASS(NotBlueprintable, NotPlaceable, HideDropdown)
class ARTChainArcProbeForTest : public AActor
{
	GENERATED_BODY()

public:
	ARTChainArcProbeForTest()
	{
		Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
		SetRootComponent(Spline);
	}

	UPROPERTY()
	TObjectPtr<USplineComponent> Spline;
};

/**
 * La stessa sonda, ma con `bReplicates` acceso nel CDO: fa le veci di un Blueprint di terzi che replica — [D-621], #4136.
 *
 * 🔴 `BP_Gadget_LinearDischarge_Arc` sta fuori da git, e chi l'ha costruito puo' averlo lasciato replicato. Con questa sonda
 * un test misura che `URTPlaybackLibrary::SpawnChainArcActor` lo spenga comunque: l'arco e' un effetto di chi guarda.
 */
UCLASS(NotBlueprintable, NotPlaceable, HideDropdown)
class ARTChainArcReplicatingProbeForTest : public ARTChainArcProbeForTest
{
	GENERATED_BODY()

public:
	ARTChainArcReplicatingProbeForTest()
	{
		bReplicates = true;
	}
};

/**
 * La stessa sonda, con DUE componenti Niagara creati per classe a runtime — #4166.
 *
 * 🔑 Il modulo Niagara non e' una dipendenza del C++ ([D-621]), quindi il tipo non si puo' nominare: la classe si cerca per nome
 * (`/Script/Niagara.NiagaraComponent`) e i componenti nascono in `OnConstruction`, che gira durante lo spawn — cioe' PRIMA che
 * `SpawnChainArcActor` ritorni, come i componenti di un Blueprint. Non nel costruttore: il CDO si costruisce al caricamento del
 * modulo, quando Niagara puo' non esserci ancora. Non in `PostInitializeComponents`: nei mondi di prova, che non sono
 * inizializzati per il gioco, non viene chiamata (misurato: l'attore nasceva con zero componenti).
 * Due e non uno, perche' la regola e' «ogni componente Niagara dell'attore». Senza il plugin non se ne crea nessuno, e un test
 * che li conta lo dice.
 */
UCLASS(NotBlueprintable, NotPlaceable, HideDropdown)
class ARTChainArcNiagaraProbeForTest : public ARTChainArcProbeForTest
{
	GENERATED_BODY()

public:
	virtual void OnConstruction(const FTransform& Transform) override
	{
		Super::OnConstruction(Transform);
		UClass* const NiagaraClass = FindObject<UClass>(nullptr, TEXT("/Script/Niagara.NiagaraComponent"));
		if (!NiagaraClass || bNiagaraCreato)
		{
			return;
		}
		bNiagaraCreato = true;
		for (int32 I = 0; I < 2; ++I)
		{
			UActorComponent* const Componente = NewObject<UActorComponent>(this, NiagaraClass, *FString::Printf(TEXT("NiagaraProbe%d"), I));
			if (USceneComponent* const Scena = Cast<USceneComponent>(Componente))
			{
				Scena->SetupAttachment(GetRootComponent());
			}
			Componente->RegisterComponent();
		}
	}

private:
	/** `OnConstruction` puo' rigirare (`RerunConstructionScripts`): i componenti si creano una volta sola. */
	bool bNiagaraCreato = false;
};
