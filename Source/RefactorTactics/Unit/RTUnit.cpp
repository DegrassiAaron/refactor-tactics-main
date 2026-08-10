#include "Unit/RTUnit.h"
#include "Turn/RTPlaybackLibrary.h" // DirectionYaw: facing planare, presentazione
#include "Map/RTHexLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTHeroData.h"
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
	SyncAbilityCooldowns();
	ApplyTeamColor();
}

void ARTUnit::ApplyCombatState(int32 NewHealth, int32 NewShield)
{
	// Solo stato LOGICO: la morte (HP<=0) non distrugge subito l'Actor. La rimozione VISIVA e la
	// distruzione sono differite al momento giusto del playback (morte visiva differita) e a fine turno,
	// cosi' il colpo mortale resta osservabile. Vedi ARTTurnManager (Defeated events / HideForDefeat).
	// Il danno assorbito dallo scudo erode PRIMA la parte temporanea: e' quella che sta per scadere comunque,
	// e cosi' lo scudo base non viene consumato finche' c'e' protezione destinata a sparire.
	const int32 ShieldLost = FMath::Max(0, Shield - FMath::Max(0, NewShield));
	TemporaryShield = FMath::Max(0, TemporaryShield - ShieldLost);

	Health = FMath::Max(0, NewHealth);
	Shield = FMath::Max(0, NewShield);
}

void ARTUnit::AddTemporaryShield(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}
	Shield += Amount;
	TemporaryShield += Amount;
}

void ARTUnit::ExpireTemporaryShield()
{
	Shield = FMath::Max(0, Shield - TemporaryShield);
	TemporaryShield = 0;
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

FString ARTUnit::ShortHeroName(FName InHeroId, const FString& Fallback)
{
	if (InHeroId.IsNone())
	{
		return Fallback;
	}
	const FString Full = InHeroId.ToString();
	// Gli HeroId sono namespaced (`Hero.Flux`): a schermo serve l'ultimo segmento. Se un giorno l'ID smettesse
	// di avere il punto, questa resta corretta invece di mostrare una stringa vuota.
	int32 Dot = INDEX_NONE;
	if (Full.FindLastChar(TEXT('.'), Dot) && Dot >= 0 && Dot + 1 < Full.Len())
	{
		return Full.RightChop(Dot + 1);
	}
	return Full;
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

void ARTUnit::PlaceOnCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight)
{
	Cell = InCell;        // posizione AUTOREVOLE (invariante #2: il FVector sotto e' solo rendering)
	PlannedCell = InCell; // dopo un movimento, il piano riparte dalla cella attuale
	PlannedPath.Reset();      // il percorso composito e' consumato
	PlannedWaypoints.Reset(); // e i suoi waypoint
	SetActorLocation(WorldForCell(InCell, Origin, HexSize, LayerHeight));
}

FVector ARTUnit::WorldForCell(const FRTCellId& InCell, const FVector& Origin, float HexSize, float LayerHeight) const
{
	// La geometria esagonale sta in URTHexLibrary (pura e testata): qui si aggiunge SOLO l'offset di
	// presentazione (VisualZOffset = 90 per il cilindro, 0 per i personaggi col pivot ai piedi).
	return URTHexLibrary::AxialToWorld(InCell, Origin, HexSize, LayerHeight)
		+ FVector(0.f, 0.f, VisualZOffset);
}

void ARTUnit::SetVisualLocation(const FVector& World)
{
	// Facing opzionale: orienta l'unita' verso la direzione di spostamento (solo presentazione, solo yaw).
	if (bFaceMovementDirection)
	{
		const FVector Current = GetActorLocation();
		if ((World - Current).SizeSquared2D() > UE_KINDA_SMALL_NUMBER)
		{
			SetActorRotation(FRotator(0.f, URTPlaybackLibrary::DirectionYaw(Current, World), 0.f));
		}
	}
	SetActorLocation(World); // solo presentazione: lo stato logico (Cell) resta invariato
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
	// Catalogo terreni §2: `Burning` e' "rimosso da `Wet`". La regola sta QUI e non nel chiamante, cosi'
	// vale per ogni sorgente di bagnato — acqua bassa, `Riva.PressureJet`, `CircularTide` — invece che nel
	// punto che si e' ricordato di scriverla. L'ordine e' voluto: si spegne anche se il Wet arriva insieme.
	if (Tag == TAG_Status_Wet)
	{
		StatusTurns.Remove(TAG_Status_Burning);
	}

	if (Turns == PersistentWhileOnCell)
	{
		CellBoundStatuses.Add(Tag); // scade quando l'unita' lascia la cella, non a tempo
		return;
	}
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
	return (Turns && *Turns > 0) || CellBoundStatuses.Contains(Tag);
}

TArray<FName> ARTUnit::GetActiveStatusNames() const
{
	TArray<FName> Names;

	// Solo gli status con durata RESIDUA: uno scaduto e' assente, non presente a zero — altrimenti il
	// checksum distinguerebbe due stati che il gioco considera identici.
	for (const TPair<FGameplayTag, int32>& Pair : StatusTurns)
	{
		if (Pair.Value > 0)
		{
			Names.AddUnique(Pair.Key.GetTagName());
		}
	}
	for (const FGameplayTag& Tag : CellBoundStatuses)
	{
		Names.AddUnique(Tag.GetTagName());
	}

	// `TMap`/`TSet` non hanno un ordine di iterazione stabile: senza questo sort il risultato dipenderebbe
	// dall'ordine di inserimento, e due esecuzioni identiche darebbero digest diversi (invariante #4).
	Names.Sort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });
	return Names;
}

void ARTUnit::ApplyMarkedBy(int32 MarkerTeamId, int32 Turns)
{
	// L'ultimo marchio vince: due squadre non possono rivendicare lo stesso bersaglio, e con 2 squadre in
	// v0.1 il caso non e' nemmeno raggiungibile (si marca un nemico, non un alleato).
	MarkedByTeam = MarkerTeamId;
	ApplyStatus(TAG_Status_Marked, Turns);
}

bool ARTUnit::RemoveStatus(FGameplayTag Tag)
{
	if (!HasStatus(Tag))
	{
		return false; // scaduto o mai applicato: non c'era nulla da purificare
	}
	if (Tag == TAG_Status_Marked)
	{
		MarkedByTeam = INDEX_NONE; // il marchio se ne va con la sua provenienza
	}
	// `Action.Cleanse` purifica lo stato, non una delle sue sorgenti: chi e' bagnato dall'acqua E da Riva
	// esce pulito da entrambe. Restare bagnati stando nell'acqua e' comunque il comportamento del turno
	// dopo, perche' la cella lo riapplica all'ingresso successivo.
	StatusTurns.Remove(Tag);
	CellBoundStatuses.Remove(Tag);
	return true;
}

void ARTUnit::RevokeCellBoundStatusesNotIn(const TSet<FGameplayTag>& Sustained)
{
	for (auto It = CellBoundStatuses.CreateIterator(); It; ++It)
	{
		if (!Sustained.Contains(*It))
		{
			It.RemoveCurrent();
		}
	}
}

void ARTUnit::TickStatuses()
{
	for (auto It = StatusTurns.CreateIterator(); It; ++It)
	{
		if (--It.Value() <= 0)
		{
			if (It.Key() == TAG_Status_Marked)
			{
				MarkedByTeam = INDEX_NONE; // scaduto senza essere speso: la provenienza scade con lui
			}
			It.RemoveCurrent();
		}
	}
}

int32 ARTUnit::GetEffectiveMoveRange() const
{
	// Root azzera. Slow (CP 4.7) non passa piu' da qui: e' un costo per cella nel pathfinding
	// (ARTTurnManager::MakeCurrentSnapshot), non una riduzione flat del budget.
	return URTCombatLibrary::EffectiveMoveRange(MoveRange, HasStatus(TAG_Status_Root));
}

int32 ARTUnit::GetEffectiveDashRange(int32 BaseRange) const
{
	// Le mobilita' lineari (Dash/Charge/Leap/Reposition) non hanno un costo per cella da aumentare: Slow non
	// le tocca in v0.1 (dichiarato in `Action.Slow`, catalogo v0.1 §5). Solo Root le azzera.
	return URTCombatLibrary::EffectiveMoveRange(BaseRange, HasStatus(TAG_Status_Root));
}

URTActionData* ARTUnit::MakeAbility(const FString& Name, int32 Range, int32 Power, int32 Area,
	int32 Cooldown, int32 EnergyCost, FGameplayTag Status, int32 StatusDur)
{
	URTActionData* Ability = NewObject<URTActionData>(this);
	Ability->DisplayName = FText::FromString(Name);
	Ability->RangeCells = Range;
	Ability->Power = Power;
	Ability->AreaRadius = Area;
	Ability->Shape = (Area > 0) ? ERTAbilityShape::Area : ERTAbilityShape::Single;
	Ability->CooldownTurns = Cooldown;
	Ability->EnergyCost = EnergyCost;
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
	// Scatto generico: portata, ricarica e identita' vengono da `Action.Dash`, non da numeri inventati qui.
	// E' anche cio' che lo rende riconoscibile come mobilita' rapida: il gate e' la FASE dichiarata dal
	// catalogo (`URTCatalogLibrary::IsFastMovement`), non un flag booleano sull'asset.
	const FRTActionDef DashDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dash"));
	URTActionData* Scatto = MakeAbility(TEXT("Scatto"), DashDef.RangeCells, 0, 0, DashDef.CooldownTurns, 0, FGameplayTag(), 0);
	Scatto->Def = DashDef;
	Abilities.Add(Scatto);

	SyncAbilityCooldowns();
}

int32 ARTUnit::FindDashAbilityIndex() const
{
	for (int32 i = 0; i < Abilities.Num(); ++i)
	{
		if (Abilities[i] && URTCatalogLibrary::IsFastMovement(Abilities[i]->Def))
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void ARTUnit::ConfigureFromHeroData(const URTHeroData* Hero)
{
	// Fail-closed: senza dati non si configura nulla. Un numero a caso sarebbe peggio di un'unita' che
	// mantiene lo stato precedente e lascia capire, dal comportamento, che qualcosa non e' stato passato.
	if (Hero == nullptr)
	{
		return;
	}

	HeroId = Hero->HeroId;
	MaxHealth = Hero->MaxHealth;
	MoveRange = Hero->MovePoints;
	VisionRange = Hero->VisionRange;
	PushResistance = Hero->PushResistance;
	Affinity = Hero->Affinity;
	Weakness = Hero->Weakness;

	// Le azioni sono gia' URTActionData*: si copiano cosi' come sono, non si ricostruiscono con MakeAbility
	// (quella e' la via delle abilita' legacy inventate in codice, non dei dati del catalogo eroi).
	Abilities = Hero->Actions;

	// Le azioni GENERICHE (D-025) si accodano al kit: sono dell'unita' quanto le sue, e da qui in poi il
	// giocatore, il bot e l'harness le trovano tutti dalla stessa lista — nessuno dei tre ha una seconda
	// strada per arrivarci, ed e' il motivo per cui prima non erano dichiarabili da nessuno.
	//
	// IN CODA, mai in testa: l'attacco base e' SEMPRE l'indice 0 (catalogo v0.1 §"Struttura di un eroe") e
	// `PlannedAbilityIndex` e' un indice, non un ID. Metterle davanti sposterebbe in silenzio ogni piano
	// gia' scritto — compresi quelli del bot, che sceglie per indice.
	Abilities.Append(URTCatalogLibrary::MakeGenericActions(this));

	SyncAbilityCooldowns();

	// AttackRange/AttackPower restano campi dell'unita' perche' bot e TurnManager li leggono ancora da li',
	// ma il NUMERO viene dall'indice 0, che l'append qui sopra lascia dov'era.
	if (Abilities.IsValidIndex(0) && Abilities[0])
	{
		AttackRange = Abilities[0]->RangeCells;
		AttackPower = Abilities[0]->Power;
	}

	Health = MaxHealth;
}

URTActionData* ARTUnit::GetAbility(int32 Index) const
{
	return Abilities.IsValidIndex(Index) ? Abilities[Index] : nullptr;
}

void ARTUnit::SyncAbilityCooldowns()
{
	AbilityCooldowns.SetNumZeroed(Abilities.Num());
}

int32 ARTUnit::GetAbilityCooldown(int32 Index) const
{
	return AbilityCooldowns.IsValidIndex(Index) ? AbilityCooldowns[Index] : 0;
}

bool ARTUnit::CanUseAbility(int32 Index) const
{
	const URTActionData* Ability = GetAbility(Index);
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
	const URTActionData* Ability = GetAbility(Index);
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
