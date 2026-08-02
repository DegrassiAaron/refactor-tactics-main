#include "Unit/RTUnit.h"
#include "Grid/RTGridLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Ability/RTAbilityData.h"
#include "Core/RTGameplayTags.h"
#include "RefactorTactics.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Il cilindro base dell'engine e' alto ~100 uu; con scala Z 1.8 diventa ~180 (meta' = 90).
	constexpr float UnitHalfHeight = 90.f;
}

ARTUnit::ARTUnit()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetRelativeScale3D(BaseMeshScale);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void ARTUnit::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	ApplyTeamColor();
}

void ARTUnit::ApplyCombatState(int32 NewHealth, int32 NewShield)
{
	Health = FMath::Max(0, NewHealth);
	Shield = FMath::Max(0, NewShield);

	if (!IsAlive())
	{
		UE_LOG(LogRT, Log, TEXT("[RT] Unit eliminata: %s (team %d)"), *GetName(), TeamId);
		Destroy();
	}
}

void ARTUnit::ApplyTeamColor()
{
	if (!Mesh || !Mesh->GetStaticMesh())
	{
		return;
	}

	const FLinearColor TeamColor = (TeamId == 0) ? Team0Color : Team1Color;

	// Preferisci il materiale dedicato (con parametro "Color"); il materiale base
	// dell'engine non espone parametri, quindi senza M_Unit l'unita' resta grigia.
	if (UMaterialInterface* Base = UnitMaterial.LoadSynchronous())
	{
		DynMaterial = UMaterialInstanceDynamic::Create(Base, this);
		Mesh->SetMaterial(0, DynMaterial);
		DynMaterial->SetVectorParameterValue(TEXT("Color"), TeamColor);
	}
	else
	{
		UE_LOG(LogRT, Warning, TEXT("[RT] Materiale M_Unit assente: unita' senza colore-team (crea /Game/Materials/M_Unit)"));
	}
}

void ARTUnit::PlaceOnCell(const FRTGridCoord& Cell, const FVector& GridOrigin, float CellSize)
{
	GridCell = Cell;
	PlannedCell = Cell; // dopo un movimento, il piano riparte dalla cella attuale
	const FVector Center = URTGridLibrary::CellToWorld(Cell, GridOrigin, CellSize);
	SetActorLocation(Center + FVector(0.f, 0.f, UnitHalfHeight));
}

void ARTUnit::OnSelected()
{
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(BaseMeshScale * 1.15f); // ingrandisce del 15% rispetto alla base
	}
}

void ARTUnit::OnDeselected()
{
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(BaseMeshScale);
	}
}

void ARTUnit::ApplyStatus(FGameplayTag Tag, int32 Turns)
{
	if (Turns <= 0)
	{
		return;
	}
	int32& Current = StatusTurns.FindOrAdd(Tag);
	Current = FMath::Max(Current, Turns); // riapplicare non accorcia una durata piu' lunga
}

bool ARTUnit::HasStatus(FGameplayTag Tag) const
{
	const int32* Turns = StatusTurns.Find(Tag);
	return Turns && *Turns > 0;
}

void ARTUnit::TickStatuses()
{
	for (auto It = StatusTurns.CreateIterator(); It; ++It)
	{
		if (--It.Value() <= 0)
		{
			It.RemoveCurrent();
		}
	}
}

int32 ARTUnit::GetEffectiveMoveRange() const
{
	return URTCombatLibrary::EffectiveMoveRange(MoveRange, HasStatus(TAG_Status_Root), HasStatus(TAG_Status_Slow));
}

int32 ARTUnit::GetAttackRange() const
{
	return BasicAttackAbility ? BasicAttackAbility->RangeCells : AttackRange;
}

int32 ARTUnit::GetAttackPower() const
{
	return BasicAttackAbility ? BasicAttackAbility->Power : AttackPower;
}

int32 ARTUnit::GetUltimatePower() const
{
	return UltimateAbility ? UltimateAbility->Power : AttackPower * UltimateMultiplier;
}

int32 ARTUnit::GetUltimateRadius() const
{
	return UltimateAbility ? UltimateAbility->AreaRadius : UltimateRadius;
}

FGameplayTag ARTUnit::GetUltimateStatusTag() const
{
	return UltimateAbility ? UltimateAbility->StatusToApply : TAG_Status_Slow;
}

int32 ARTUnit::GetUltimateStatusDuration() const
{
	return UltimateAbility ? UltimateAbility->StatusDuration : 2;
}
