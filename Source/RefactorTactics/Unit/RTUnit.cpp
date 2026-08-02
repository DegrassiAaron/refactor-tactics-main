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
	EnsureDefaultAbilities();
	AbilityCooldowns.Init(0, Abilities.Num());
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

void ARTUnit::EnsureDefaultAbilities()
{
	if (Abilities.Num() > 0)
	{
		return;
	}

	// Attacco base: bersaglio singolo, nessun costo/cooldown; accumula energia.
	URTAbilityData* Attack = NewObject<URTAbilityData>(this, TEXT("Ability_Attack"));
	Attack->DisplayName = FText::FromString(TEXT("Attacco"));
	Attack->RangeCells = AttackRange;
	Attack->Power = AttackPower;
	Abilities.Add(Attack);

	// Colpo pesante: piu' danno, con ricarica di 2 turni.
	URTAbilityData* Heavy = NewObject<URTAbilityData>(this, TEXT("Ability_Heavy"));
	Heavy->DisplayName = FText::FromString(TEXT("Colpo pesante"));
	Heavy->RangeCells = FMath::Max(1, AttackRange - 1);
	Heavy->Power = AttackPower + 20;
	Heavy->CooldownTurns = 2;
	Abilities.Add(Heavy);

	// Ultimate: area + Slow, richiede energia piena e la consuma.
	URTAbilityData* Ult = NewObject<URTAbilityData>(this, TEXT("Ability_Ultimate"));
	Ult->DisplayName = FText::FromString(TEXT("Ultimate"));
	Ult->RangeCells = AttackRange;
	Ult->Power = AttackPower * UltimateMultiplier;
	Ult->AreaRadius = UltimateRadius;
	Ult->StatusToApply = TAG_Status_Slow;
	Ult->StatusDuration = 2;
	Ult->EnergyCost = MaxEnergy;
	Abilities.Add(Ult);
}

URTAbilityData* ARTUnit::GetAbility(int32 Index) const
{
	return Abilities.IsValidIndex(Index) ? Abilities[Index] : nullptr;
}

int32 ARTUnit::GetAbilityCooldown(int32 Index) const
{
	return AbilityCooldowns.IsValidIndex(Index) ? AbilityCooldowns[Index] : 0;
}

bool ARTUnit::CanUseAbility(int32 Index) const
{
	const URTAbilityData* Ability = GetAbility(Index);
	return Ability && URTCombatLibrary::IsAbilityUsable(GetAbilityCooldown(Index), Energy, Ability->EnergyCost);
}

void ARTUnit::SelectAbility(int32 Index)
{
	if (Abilities.IsValidIndex(Index))
	{
		SelectedAbilityIndex = Index;
	}
}

void ARTUnit::ConsumeAbility(int32 Index)
{
	const URTAbilityData* Ability = GetAbility(Index);
	if (!Ability)
	{
		return;
	}
	if (Ability->EnergyCost > 0)
	{
		Energy = FMath::Max(0, Energy - Ability->EnergyCost);
	}
	if (Ability->CooldownTurns > 0 && AbilityCooldowns.IsValidIndex(Index))
	{
		AbilityCooldowns[Index] = Ability->CooldownTurns;
	}
}

void ARTUnit::TickCooldowns()
{
	for (int32& CD : AbilityCooldowns)
	{
		if (CD > 0)
		{
			--CD;
		}
	}
}
