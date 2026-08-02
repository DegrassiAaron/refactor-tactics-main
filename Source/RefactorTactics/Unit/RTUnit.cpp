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

URTAbilityData* ARTUnit::MakeAbility(const FString& Name, int32 Range, int32 Power, int32 Area,
	int32 Cooldown, int32 EnergyCost, FGameplayTag Status, int32 StatusDur)
{
	URTAbilityData* Ability = NewObject<URTAbilityData>(this);
	Ability->DisplayName = FText::FromString(Name);
	Ability->RangeCells = Range;
	Ability->Power = Power;
	Ability->AreaRadius = Area;
	Ability->Shape = (Area > 0) ? ERTAbilityShape::Area : ERTAbilityShape::Single;
	Ability->CooldownTurns = Cooldown;
	Ability->EnergyCost = EnergyCost;
	Ability->StatusToApply = Status;
	Ability->StatusDuration = StatusDur;
	return Ability;
}

void ARTUnit::EnsureDefaultAbilities()
{
	if (Abilities.Num() > 0)
	{
		return;
	}
	Abilities.Add(MakeAbility(TEXT("Attacco"), AttackRange, AttackPower, 0, 0, 0, FGameplayTag(), 0));
	Abilities.Add(MakeAbility(TEXT("Colpo pesante"), FMath::Max(1, AttackRange - 1), AttackPower + 20, 0, 2, 0, FGameplayTag(), 0));
	Abilities.Add(MakeAbility(TEXT("Ultimate"), AttackRange, AttackPower * UltimateMultiplier, UltimateRadius, 0, MaxEnergy, TAG_Status_Slow, 2));
}

void ARTUnit::ConfigureAsArchetype(ERTArchetype InArchetype)
{
	Archetype = InArchetype;
	Abilities.Reset();

	if (InArchetype == ERTArchetype::Ranger)
	{
		MaxHealth = 80;  Shield = 0;   MoveRange = 5;  AttackRange = 6;  AttackPower = 25;
		BaseMeshScale = FVector(1.0f, 1.0f, 2.0f); // snello e alto
		Abilities.Add(MakeAbility(TEXT("Tiro"), 6, 25, 0, 0, 0, FGameplayTag(), 0));
		// Colpo preciso: perfora la traiettoria e "rivela" il bersaglio (intento visibile per 1 turno).
		URTAbilityData* Precise = MakeAbility(TEXT("Colpo preciso"), 7, 40, 0, 2, 0, TAG_Status_Reveal, 2);
		Precise->Shape = ERTAbilityShape::Line;
		Abilities.Add(Precise);
		Abilities.Add(MakeAbility(TEXT("Raffica"), 6, 50, 1, 0, MaxEnergy, TAG_Status_Slow, 2)); // AoE + Slow
	}
	else // Guardian
	{
		MaxHealth = 140; Shield = 20;  MoveRange = 3;  AttackRange = 3;  AttackPower = 30;
		BaseMeshScale = FVector(1.5f, 1.5f, 1.6f); // tozzo e largo
		URTAbilityData* Sweep = MakeAbility(TEXT("Spazzata"), 3, 30, 0, 0, 0, FGameplayTag(), 0);
		Sweep->Shape = ERTAbilityShape::Cone; // colpisce a ventaglio davanti
		Abilities.Add(Sweep);
		URTAbilityData* Barrier = MakeAbility(TEXT("Barriera"), 0, 40, 0, 3, 0, FGameplayTag(), 0);
		Barrier->bSelfTarget = true; // supporto: +40 scudo su se stessi (fase Prep)
		Abilities.Add(Barrier);
		Abilities.Add(MakeAbility(TEXT("Terremoto"), 3, 40, 2, 0, MaxEnergy, TAG_Status_Root, 2)); // AoE ampio + Root
	}

	Health = MaxHealth;
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(BaseMeshScale);
	}
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
