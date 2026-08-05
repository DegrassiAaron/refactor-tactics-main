#include "Unit/RTUnit.h"
#include "Grid/RTGridLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Ability/RTAbilityData.h"
#include "Core/RTGameplayTags.h"
#include "RefactorTactics.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

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

	// Anello di team a terra: figlio del root, scala ASSOLUTA (non eredita la deformazione ne' la selezione del
	// cilindro), senza collisione. Nascosto finche' ApplyTeamColor non trova un materiale team.
	TeamRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeamRing"));
	TeamRing->SetupAttachment(Mesh);
	TeamRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TeamRing->SetUsingAbsoluteScale(true);
	TeamRing->SetVisibility(false);
	if (CylinderMesh.Succeeded())
	{
		TeamRing->SetStaticMesh(CylinderMesh.Object);
	}
	TeamRing->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.02f)); // disco piatto, raggio ~80 cm

	// Anello di SELEZIONE: gemello del TeamRing, piu' grande (cornice esterna) per distinguersi. Nascosto
	// finche' l'unita' non e' selezionata (e finche' ApplyTeamColor non trova un materiale di selezione).
	SelectionRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
	SelectionRing->SetupAttachment(Mesh);
	SelectionRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionRing->SetUsingAbsoluteScale(true);
	SelectionRing->SetVisibility(false);
	if (CylinderMesh.Succeeded())
	{
		SelectionRing->SetStaticMesh(CylinderMesh.Object);
	}
	SelectionRing->SetRelativeScale3D(FVector(1.9f, 1.9f, 0.02f)); // cornice esterna al TeamRing (1.6)
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
	// Solo stato LOGICO: la morte (HP<=0) non distrugge subito l'Actor. La rimozione VISIVA e la
	// distruzione sono differite al momento giusto del playback (morte visiva differita) e a fine turno,
	// cosi' il colpo mortale resta osservabile. Vedi ARTTurnManager (Defeated events / HideForDefeat).
	Health = FMath::Max(0, NewHealth);
	Shield = FMath::Max(0, NewShield);
}

void ARTUnit::HideForDefeat()
{
	// Morte visiva: nasconde la mesh e disabilita la collisione. Lo stato logico e' gia' HP=0;
	// la distruzione effettiva dell'Actor avviene a fine turno (ARTTurnManager::ConcludeTurn).
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

FLinearColor ARTUnit::TeamColorFor(int32 InTeamId, const FLinearColor& Team0, const FLinearColor& Team1)
{
	return (InTeamId == 0) ? Team0 : Team1;
}

float ARTUnit::RingLocalZ(float VisualZOffset, float ParentScaleZ)
{
	// La posizione relativa Z del figlio e' scalata dalla scala Z del genitore: compensa VisualZOffset
	// dividendo per quella scala, +1 per risalire dal centro-base al piano. Guardia: scala 0 -> niente divisione.
	return (ParentScaleZ != 0.f) ? (-VisualZOffset / ParentScaleZ) + 1.f : 1.f;
}

void ARTUnit::ApplyTeamColor()
{
	const FLinearColor TeamColor = TeamColorFor(TeamId, Team0Color, Team1Color);

	// Cilindro segnaposto: MID con parametro "Color" (senza M_Unit resta grigio).
	if (Mesh && Mesh->GetStaticMesh())
	{
		if (UMaterialInterface* Base = UnitMaterial.LoadSynchronous())
		{
			DynMaterial = UMaterialInstanceDynamic::Create(Base, this);
			Mesh->SetMaterial(0, DynMaterial);
			DynMaterial->SetVectorParameterValue(TEXT("Color"), TeamColor);
		}
		else
		{
			UE_LOG(LogRT, Warning, TEXT("[RT] Materiale del tint assente: unita' senza colore-team (crea /Game/RT/Art/GlobalMaterials/M_Global_Tint)"));
		}
	}

	// Anello di team a terra: compensa VisualZOffset e la scala Z del genitore per restare a livello cella.
	// Colorato se M_TeamRing c'e', altrimenti nascosto (fallback: resta il colore sul cilindro).
	if (TeamRing)
	{
		const float RingZ = RingLocalZ(VisualZOffset, BaseMeshScale.Z);
		TeamRing->SetRelativeLocation(FVector(0.f, 0.f, RingZ));
		if (UMaterialInterface* RingBase = TeamRingMaterial.LoadSynchronous())
		{
			RingDynMaterial = UMaterialInstanceDynamic::Create(RingBase, this);
			TeamRing->SetMaterial(0, RingDynMaterial);
			RingDynMaterial->SetVectorParameterValue(TEXT("Color"), TeamColor);
			TeamRing->SetVisibility(true);
		}
		else
		{
			TeamRing->SetVisibility(false);
		}
	}

	// Anello di selezione: stessa quota-terra del TeamRing, colore di selezione. Resta NASCOSTO finche'
	// OnSelected non lo mostra. Senza materiale di selezione non compare (fallback come il TeamRing).
	if (SelectionRing)
	{
		SelectionRing->SetRelativeLocation(FVector(0.f, 0.f, RingLocalZ(VisualZOffset, BaseMeshScale.Z)));
		if (UMaterialInterface* SelBase = SelectionRingMaterial.LoadSynchronous())
		{
			SelectionRingDynMaterial = UMaterialInstanceDynamic::Create(SelBase, this);
			SelectionRing->SetMaterial(0, SelectionRingDynMaterial);
			SelectionRingDynMaterial->SetVectorParameterValue(TEXT("Color"), SelectionColor);
		}
		SelectionRing->SetVisibility(false);
	}
}

void ARTUnit::PlaceOnCell(const FRTGridCoord& Cell, const FVector& GridOrigin, float CellSize, float LayerHeight)
{
	GridCell = Cell;
	PlannedCell = Cell; // dopo un movimento, il piano riparte dalla cella attuale
	PlannedPath.Reset();      // il percorso composito e' consumato
	PlannedWaypoints.Reset(); // e i suoi waypoint
	SetActorLocation(WorldForCell(Cell, GridOrigin, CellSize, LayerHeight));
}

FVector ARTUnit::WorldForCell(const FRTGridCoord& Cell, const FVector& GridOrigin, float CellSize, float LayerHeight) const
{
	// Delegato alla utility pura (testata): VisualZOffset = 90 per il cilindro, 0 per personaggi (pivot ai piedi).
	return URTGridLibrary::CellToWorldElevated(Cell, GridOrigin, CellSize, VisualZOffset, LayerHeight);
}

void ARTUnit::SetVisualLocation(const FVector& World)
{
	// Facing opzionale: orienta l'unita' verso la direzione di spostamento (solo presentazione, solo yaw).
	if (bFaceMovementDirection)
	{
		const FVector Current = GetActorLocation();
		if ((World - Current).SizeSquared2D() > UE_KINDA_SMALL_NUMBER)
		{
			SetActorRotation(FRotator(0.f, URTGridLibrary::DirectionYaw(Current, World), 0.f));
		}
	}
	SetActorLocation(World); // solo presentazione: lo stato logico (GridCell) resta invariato
}

void ARTUnit::OnSelected()
{
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(BaseMeshScale * 1.15f); // cilindro segnaposto: ingrandisce del 15%
	}
	// Anello di selezione a terra: riscontro visibile anche sugli skeletal (dove il cilindro e' nascosto).
	// Compare solo se un materiale di selezione e' stato assegnato (MID creato in ApplyTeamColor).
	if (SelectionRing && SelectionRingDynMaterial)
	{
		SelectionRing->SetVisibility(true);
	}
}

void ARTUnit::OnDeselected()
{
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(BaseMeshScale);
	}
	if (SelectionRing)
	{
		SelectionRing->SetVisibility(false);
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

int32 ARTUnit::GetEffectiveDashRange(int32 BaseRange) const
{
	// Stessa logica del movimento (Root -> 0, Slow -> meta'), applicata alla portata dello scatto.
	return URTCombatLibrary::EffectiveMoveRange(BaseRange, HasStatus(TAG_Status_Root), HasStatus(TAG_Status_Slow));
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
	// Scatto generico (fase Dash): riposizionamento rapido oltre il range di movimento, ricarica 2 turni.
	URTAbilityData* Scatto = MakeAbility(TEXT("Scatto"), MoveRange + 2, 0, 0, 2, 0, FGameplayTag(), 0);
	Scatto->bDash = true;
	Abilities.Add(Scatto);
}

int32 ARTUnit::FindDashAbilityIndex() const
{
	for (int32 i = 0; i < Abilities.Num(); ++i)
	{
		if (Abilities[i] && Abilities[i]->bDash)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void ARTUnit::ConfigureAsArchetype(ERTArchetype InArchetype)
{
	Archetype = InArchetype;
	Abilities.Reset();

	if (InArchetype == ERTArchetype::Ranger)
	{
		MaxHealth = 80;  Shield = 0;   MoveRange = 5;  AttackRange = 6;  AttackPower = 25;
		KiteStandoff = 4; // fragile e a distanza: arretra se un nemico entro 4 celle e nessun tiro pronto
		BaseMeshScale = FVector(1.0f, 1.0f, 2.0f); // snello e alto
		Abilities.Add(MakeAbility(TEXT("Tiro"), 6, 25, 0, 0, 0, FGameplayTag(), 0));
		// Colpo preciso: perfora la traiettoria e "rivela" il bersaglio (intento visibile per 1 turno).
		URTAbilityData* Precise = MakeAbility(TEXT("Colpo preciso"), 7, 40, 0, 2, 0, TAG_Status_Reveal, 2);
		Precise->Shape = ERTAbilityShape::Line;
		Abilities.Add(Precise);
		URTAbilityData* Raffica = MakeAbility(TEXT("Raffica"), 6, 50, 1, 0, MaxEnergy, TAG_Status_Slow, 2); // AoE + Slow
		Raffica->bIgnites = true; // raffica infuocata: incendia il terreno infiammabile nell'area
		Abilities.Add(Raffica);
		// Scatto: riposizionamento rapido (fase Dash), 5 celle, ricarica 2 turni. Il Ranger e' mobile.
		URTAbilityData* Scatto = MakeAbility(TEXT("Scatto"), 5, 0, 0, 2, 0, FGameplayTag(), 0);
		Scatto->bDash = true;
		Abilities.Add(Scatto);
	}
	else // Guardian
	{
		MaxHealth = 140; Shield = 20;  MoveRange = 3;  AttackRange = 3;  AttackPower = 30;
		KiteStandoff = 0; // mischia: non fa kiting, punta a chiudere la distanza
		BaseMeshScale = FVector(1.5f, 1.5f, 1.6f); // tozzo e largo
		URTAbilityData* Sweep = MakeAbility(TEXT("Spazzata"), 3, 30, 0, 0, 0, FGameplayTag(), 0);
		Sweep->Shape = ERTAbilityShape::Cone; // colpisce a ventaglio davanti
		Sweep->bKnockback = true;             // e RESPINGE i colpiti di 2 celle (spinge oltre il bordo/nella lava)
		Sweep->KnockbackDistance = 2;
		Abilities.Add(Sweep);
		URTAbilityData* Barrier = MakeAbility(TEXT("Barriera"), 0, 40, 0, 3, 0, FGameplayTag(), 0);
		Barrier->bSelfTarget = true; // supporto: +40 scudo su se stessi (fase Prep)
		Abilities.Add(Barrier);
		Abilities.Add(MakeAbility(TEXT("Terremoto"), 3, 40, 2, 0, MaxEnergy, TAG_Status_Root, 2)); // AoE ampio + Root
		// Carica: scatto d'irruzione (fase Dash), 4 celle per chiudere la distanza, ricarica 3 turni.
		URTAbilityData* Carica = MakeAbility(TEXT("Carica"), 4, 0, 0, 3, 0, FGameplayTag(), 0);
		Carica->bDash = true;
		Abilities.Add(Carica);
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
