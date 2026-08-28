#include "Unit/RTUnit.h"
#include "Turn/RTReactionOpportunityTypes.h" // IsDeclaredConditionAllowed: la validazione sta in un posto solo
#include "Turn/RTPlaybackLibrary.h" // DirectionYaw: facing planare, presentazione
#include "Map/RTHexLibrary.h"
#include "Combat/RTCombatLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h"
#include "Ability/RTEquipmentData.h" // ERTEquipmentSlot: `EquipLoadout` distingue chi MODIFICA da chi CONCEDE
#include "Ability/RTHeroData.h"
#include "Core/RTGameplayTags.h"
#include "RefactorTactics.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Unit/RTUnitAnimInstance.h"
#include "Perception/RTTeamKnowledge.h" // ContactLifetimeTurns: la durata del ricordo ha un owner, non si ricopia

ARTUnit::ARTUnit()
{
	PrimaryActorTick.bCanEverTick = false;

	// [D-224]: lo scudo base esiste da subito, e sta nel COSTRUTTORE per una ragione misurata — i mondi
	// di test (`UWorld::CreateWorld` senza `BeginPlay`) non fanno partire `BeginPlay`, quindi metterlo li'
	// lo avrebbe reso invisibile a meta' della suite: presente in partita, assente dove lo si verifica.
	// Il costruttore gira su `NewObject` come su ogni `SpawnActor`, e non ha quel buco.
	RechargeBaseShield();

	// ROOT NEUTRO (#593). Il root non porta scala, e questa e' l'unica proprieta' che conta: **qualunque
	// componente aggiunto in Blueprint eredita la scala del root**. Finche' il root era il cilindro
	// segnaposto — `(1.2, 1.2, 1.8)` — una Skeletal Mesh attaccata sotto veniva stirata di `1.8/1.2 = 1.5x`.
	//
	// ⚠️ **Questo NON basta a raddrizzare i quattro `BP_Unit_*` gia' esistenti**, ed e' il limite da
	// dichiarare invece di lasciare credere il contrario. I loro nodi SCS dichiarano il proprio genitore
	// **per nome** (`ParentComponentOrVariableName` + `bIsParentComponentNative`, presenti nella name table
	// di tutti e quattro): finche' un componente nativo chiamato `Mesh` esiste — e esiste — la skeletal
	// resta figlia di lui, che porta ancora `BaseMeshScale`. Riparentarla sotto `SceneRoot` e' un'apertura
	// di editor: seduta **U7**, con Binary Asset Lease sui `.uasset`. Da qui si toglie la CAUSA per tutto
	// cio' che nasce d'ora in poi; i quattro esistenti li raddrizza una persona.
	//
	// 🔴 **Il precedente nel progetto e' UNO, non due**: `ARTCameraPawn.cpp:18` crea un `USceneComponent`
	// chiamato «Root» e lo imposta come radice. `ARTHexMapActor.cpp:91` **no** — usa un
	// `UInstancedStaticMeshComponent` (`Cells`) come root, cioe' lo stesso schema che questa modifica sta
	// togliendo da qui. Una stesura precedente citava entrambi, e la code review l'ha falsificata: chi
	// verificasse il precedente per decidere come radicare un actor nuovo troverebbe la risposta opposta.
	//
	// (Su `ARTHexMapActor` la scelta e' difendibile e non si tocca qui: il suo root non porta scala
	// deformante, e nessuno gli attacca componenti da Blueprint.)
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetRelativeScale3D(BaseMeshScale);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
	}

	// Anello di team a terra: figlio del ROOT NEUTRO, non piu' del cilindro. Il cambio non e' cosmetico —
	// da figlio della mesh, la sua posizione relativa Z veniva moltiplicata per `BaseMeshScale.Z`, ed e' la
	// ragione per cui `RingLocalZ` aveva un parametro `ParentScaleZ` da compensare. Sotto un root unitario
	// non c'e' piu' niente da compensare, e l'anello smette di dipendere dall'aspetto del segnaposto.
	//
	// `SetUsingAbsoluteScale` resta, ma **non e' piu' la stessa difesa** e vale dirlo invece di lasciarlo
	// credere ridondante. Contro l'ingrandimento del 15% alla selezione non serve piu': quello tocca
	// `Mesh` (`OnSelected`, riga 266), che ora e' un FRATELLO dell'anello, non il suo genitore — la
	// gerarchia lo ferma da sola. Cio' che continua a coprire e' un caso diverso: la scala dell'ATTORE,
	// che e' la scala del root, e che chiunque puo' cambiare per istanza in livello o in Blueprint.
	//
	// ⚠️ E la copre a meta', per come e' fatto `bAbsoluteScale`: `CalcNewComponentToWorld` copia la
	// scala relativa ma lascia la traslazione moltiplicata dal genitore. Su un attore scalato l'anello
	// mantiene la propria DIMENSIONE e sposta la propria QUOTA. Non e' un difetto introdotto qui — vale
	// identico prima di #593 — e raddrizzarlo vorrebbe dire scegliere fra due comportamenti, cioe' una
	// decisione, non una riga.
	TeamRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeamRing"));
	TeamRing->SetupAttachment(SceneRoot);
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
	SelectionRing->SetupAttachment(SceneRoot);
	SelectionRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionRing->SetUsingAbsoluteScale(true);
	SelectionRing->SetVisibility(false);
	if (CylinderMesh.Succeeded())
	{
		SelectionRing->SetStaticMesh(CylinderMesh.Object);
	}
	SelectionRing->SetRelativeScale3D(FVector(1.9f, 1.9f, 0.02f)); // cornice esterna al TeamRing (1.6)

	// Il grafo di locomozione vive in C++ (`#288`): nessun `.uasset` da duplicare, e le clip per eroe sono
	// dati versionati invece che grafi dentro quattro binari da ~700 KB.
	UnitAnimClass = URTUnitAnimInstance::StaticClass();

	// Freccia di orientamento: figlia del ROOT, quindi segue l'attore e non la mesh. E' la differenza fra
	// le due che dice se `MeshYawOffset` e' giusto.
	FacingArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingArrow"));
	FacingArrow->SetupAttachment(SceneRoot);
	FacingArrow->SetUsingAbsoluteScale(true);
	FacingArrow->ArrowSize = 1.2f;
	FacingArrow->ArrowLength = 90.f;
	FacingArrow->ArrowColor = FColor(255, 210, 30);
	FacingArrow->SetHiddenInGame(false); // di default un ArrowComponent si vede solo in editor

	// Sagoma dell'ultimo contatto (Task 6): SEPARATA dagli anelli e dalla freccia, che seguono l'attore.
	// `SetUsingAbsoluteLocation`/`Rotation` la sganciano dal transform del padre — un componente figlio
	// normale erediterebbe il transform e la trascinerebbe con l'unita' vera, che nel frattempo puo' essersi
	// mossa altrove. La aggiorna solo `UpdateContactGhost`, mai `RefreshComponentVisibility` (che nasconde il
	// personaggio vero) ne' `ApplyTeamColor`/`ApplyFacingArrow` (che sono presentazione del vivo).
	ContactGhost = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ContactGhost"));
	ContactGhost->SetupAttachment(SceneRoot);
	ContactGhost->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ContactGhost->SetCastShadow(false);
	ContactGhost->SetUsingAbsoluteLocation(true);
	ContactGhost->SetUsingAbsoluteRotation(true);
	ContactGhost->SetVisibility(false); // niente finche' non c'e' un ricordo da mostrare (UpdateContactGhost)
}

void ARTUnit::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	EnsureDefaultAbilities();
	SyncAbilityCooldowns();
	ApplyTeamColor();
	ApplyUnitAnimClass();
	ApplyMeshYawOffset();
	ApplyFacingArrow();
}

void ARTUnit::ApplyUnitAnimClass()
{
	if (UnitAnimClass == nullptr)
	{
		return;
	}

	// La Skeletal Mesh la aggiunge il Blueprint, non il C++: il cilindro segnaposto e' uno
	// `UStaticMeshComponent`. Un'unita' senza skeletal — il ripiego di `#287` — non ha niente da animare,
	// e questo metodo non fa nulla.
	TArray<USkeletalMeshComponent*> Skeletals;
	GetComponents<USkeletalMeshComponent>(Skeletals);

	for (USkeletalMeshComponent* Skeletal : Skeletals)
	{
		if (Skeletal == nullptr || Skeletal->GetSkeletalMeshAsset() == nullptr)
		{
			continue;
		}

		// ⚠️ **Una `Anim Class` gia' scelta in Blueprint VINCE.** Assegnare comunque toglierebbe la via per
		// provare un AnimBP a mano su un personaggio, che e' esattamente cio' che si fa quando si valuta
		// se il grafo di un pack basta.
		if (Skeletal->GetAnimClass() != nullptr)
		{
			continue;
		}

		Skeletal->SetAnimInstanceClass(UnitAnimClass);
	}
}

void ARTUnit::ApplyMeshYawOffset()
{
	// La compensazione che `ACharacter` fa nel proprio costruttore e che qui non c'era: la skeletal e'
	// modellata rivolta lungo +Y, il forward dell'attore e' +X.
	TArray<USkeletalMeshComponent*> Skeletals;
	GetComponents<USkeletalMeshComponent>(Skeletals);
	for (USkeletalMeshComponent* Skeletal : Skeletals)
	{
		if (Skeletal != nullptr && Skeletal->GetSkeletalMeshAsset() != nullptr)
		{
			Skeletal->SetRelativeRotation(FRotator(0.f, MeshYawOffset, 0.f));
		}
	}
}

void ARTUnit::ApplyFacingArrow()
{
	if (FacingArrow == nullptr)
	{
		return;
	}
	FacingArrow->SetHiddenInGame(!bShowFacingArrow);

	// A terra come gli anelli, e sopra il disco della cella per la stessa ragione: sotto `RTCellTopZ`
	// finirebbe DENTRO il disco e non si vedrebbe.
	FacingArrow->SetRelativeLocation(FVector(0.f, 0.f, TeamRingLocalZ(VisualZOffset) + RingStackSeparation));
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

void ARTUnit::RechargeBaseShield()
{
	// Un'unita' abbattuta non si ricarica: il suo stato logico e' finale finche' il turno non la rimuove,
	// e uno scudo su un cadavere comparirebbe nell'hash di fine partita.
	if (Health <= 0)
	{
		return;
	}
	Shield = URTCombatLibrary::BaseShield + TemporaryShield;
}

void ARTUnit::HideForDefeat()
{
	// Morte visiva: nasconde la mesh e disabilita la collisione. Lo stato logico e' gia' HP=0;
	// la distruzione effettiva dell'Actor avviene a fine turno (ARTTurnManager::ConcludeTurn).
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

bool ARTUnit::ShouldBeRendered(bool bAlive, bool bKnownToObserver)
{
	return bAlive && bKnownToObserver;
}

void ARTUnit::SetKnownToObserver(bool bKnown)
{
	if (bKnownToObserver == bKnown)
	{
		return; // niente churn di stato render a ogni frame
	}
	bKnownToObserver = bKnown;
	RefreshComponentVisibility();
}

bool ARTUnit::ShouldShowPlaceholderMesh(bool bRender, bool bHasHeroMesh)
{
	return bRender && !bHasHeroMesh;
}

bool ARTUnit::ShouldShowSelectionRing(bool bRender, bool bSelected, bool bHasSelectionMaterial)
{
	return bRender && bSelected && bHasSelectionMaterial;
}

bool ARTUnit::ShouldShowTeamRing(bool bRender, bool bHasTeamRingMaterial)
{
	return bRender && bHasTeamRingMaterial;
}

void ARTUnit::RefreshComponentVisibility()
{
	const bool bRender = ShouldBeRendered(IsAlive(), bKnownToObserver);

	// La skeletal arriva dal Blueprint `BP_Unit_*`, non dal C++: si cerca fra i componenti, escludendo
	// SEMPRE `ContactGhost` (Task 6) per identita' — vedi `FindHeroSkeletal`. Cercata UNA volta: serve sia
	// a mostrarla sia a decidere del cilindro segnaposto qui sotto.
	USkeletalMeshComponent* HeroSkeletal = FindHeroSkeletal();
	const bool bHasHeroMesh = HeroSkeletal != nullptr && HeroSkeletal->GetSkeletalMeshAsset() != nullptr;

	// 🔴 Si nascondono i COMPONENTI, non l'actor. `SetActorHiddenInGame` propaga a tutti i componenti,
	// sagoma dell'ultimo contatto compresa (Task 6) — che deve vedersi proprio quando l'unita' non si vede.
	//
	// 🔴 **Ogni riga qui e' una FUNZIONE dello stato, mai un'assegnazione che sovrascrive un altro owner.**
	// La forma precedente accendeva `Mesh` e `SelectionRing` incondizionatamente su `bRender`, e quel
	// «true» clobberava due decisioni prese altrove: il cilindro segnaposto nascosto sugli eroi skeletal
	// tornava dentro il personaggio, e l'anello di selezione si accendeva su un nemico che nessuno aveva
	// selezionato — bastava perderlo di vista e riavvistarlo. Con W scrittori e F flag servirebbero W×F
	// congiunzioni sparse, e ognuna e' una che qualcuno dimentichera': qui i flag sono lo STATO e questa
	// funzione e' l'unico posto che sa quali componenti esistono.
	if (Mesh)
	{
		// ⚠️ Il cilindro e' un SEGNAPOSTO, e il posto non e' piu' vuoto quando l'eroe ha la sua skeletal.
		// Il predicato si CALCOLA da qui invece di rileggere cio' che il Blueprint ha impostato.
		//
		// 🔴 **`SetVisibility` non scavalca `bHiddenInGame`** — una stesura precedente di questo commento
		// affermava che «non conta» quale delle due forme il Blueprint usi, ed era falso in generale. Sono
		// due flag distinti (si disegna solo con `bVisible && !bHiddenInGame`). Qui regge per una ragione
		// piu' stretta: **sugli eroi il predicato vale `false` in entrambi i casi**, perche' `bHasHeroMesh`
		// e' vero e il cilindro va nascosto comunque — le due forme coincidono per VERSO. Su un'unita'
		// senza skeletal il cui `BP_Unit_*` usasse `bHiddenInGame = true`, questa riga chiederebbe di
		// mostrare il cilindro e il cilindro resterebbe invisibile. Vedi `ShouldShowPlaceholderMesh`.
		Mesh->SetVisibility(ShouldShowPlaceholderMesh(bRender, bHasHeroMesh), /*bPropagateToChildren*/ false);
	}

	// L'anello di squadra esiste solo se `ApplyTeamColor` ha trovato il materiale: senza, il ripiego e' il
	// colore sul cilindro, e accenderlo mostrerebbe un disco grigio che non dice niente.
	if (TeamRing)
	{
		TeamRing->SetVisibility(ShouldShowTeamRing(bRender, RingDynMaterial != nullptr), false);
	}

	// L'anello di selezione dipende dalla SELEZIONE, che e' uno stato di questa unita' e non si legge da
	// `IsVisible()`: dopo un `RefreshComponentVisibility` con `bRender == false` quella risponderebbe «no»
	// anche su un'unita' selezionata, e la selezione si perderebbe al primo riavvistamento.
	if (SelectionRing)
	{
		SelectionRing->SetVisibility(
			ShouldShowSelectionRing(bRender, bSelected, SelectionRingDynMaterial != nullptr), false);
	}

	// La freccia di facing ha il proprio interruttore di presentazione.
	if (FacingArrow)   { FacingArrow->SetVisibility(bRender && bShowFacingArrow, false); }

	if (HeroSkeletal)
	{
		HeroSkeletal->SetVisibility(bRender, false);
	}

	// La collisione si spegne sull'ACTOR: `SetVisibility` non la tocca, e l'unico proxy di click e' `Mesh`
	// (QueryOnly + ECR_Block su tutti i canali). Un'unita' invisibile ma cliccabile e' peggio di una
	// visibile: il giocatore selezionerebbe qualcosa che non vede. La sagoma e' `NoCollision`, quindi
	// spegnere la collisione dell'actor non la riguarda.
	//
	// ⚠️ Resta su `bRender` e NON sul cilindro: il proxy di click deve rispondere anche su un eroe skeletal,
	// dove il cilindro e' nascosto ma continua a fare da forma di collisione.
	SetActorEnableCollision(bRender);
}

USkeletalMeshComponent* ARTUnit::FindHeroSkeletal() const
{
	TArray<USkeletalMeshComponent*> Skeletals;
	GetComponents<USkeletalMeshComponent>(Skeletals);
	for (USkeletalMeshComponent* Skeletal : Skeletals)
	{
		if (Skeletal != nullptr && Skeletal != ContactGhost)
		{
			return Skeletal;
		}
	}
	return nullptr;
}

float ARTUnit::GhostOpacityForContact(int32 ContactTurn, int32 CurrentTurn)
{
	const int32 Age = CurrentTurn - ContactTurn;
	if (Age < 0 || Age > URTTeamKnowledgeLibrary::ContactLifetimeTurns)
	{
		return 0.0f;
	}
	// 🔴 **`0.75`, non `1.0`.** La spec dichiara che il ricordo e' una sagoma **semitrasparente**
	// (`docs/technical/systems/conoscenza-parziale-visibile-spec.md`, decisione **S4** in §2, ripetuta
	// dall'emendamento a `progettazione-hud.md` in §7). ⚠️ Il riferimento NON e' §4 A4, che dichiara altri
	// due canali — *monocromo desaturato* e *senza freccia di facing* — e distingue la sagoma dall'Action
	// Ghost, non da un'unita' viva: chi venisse a rileggere A4 non ci troverebbe questa regola.
	//
	// `1.0` e' opaco, e `Age == 0` — «perso di vista in QUESTO
	// turno» — e' il caso piu' frequente, non un angolo. Finche' l'unita' vera restava disegnata accanto
	// alla sagoma l'opacita' era cosmetica; da quando l'unita' ignota sparisce, la sagoma e' l'unica cosa
	// che il giocatore vede di quel nemico e sta nella cella dell'ULTIMO CONTATTO, non in quella vera:
	// dichiararla «non e' piu' li'» e' il lavoro della trasparenza.
	//
	// Il valore: piu' leggibile di `0.45` perche' il contatto e' fresco, e abbastanza sotto `1.0` da far
	// vedere il terreno attraverso la sagoma. ⚠️ **Non e' il canale portante** — nome, barra HP, anello di
	// squadra, anello di selezione e freccia di facing sono gia' spenti su un'unita' ignota, e sono loro a
	// distinguere il ricordo da un'unita' viva. E' il canale **dichiarato**, ed era violato.
	return (Age == 0) ? 0.75f : 0.45f;
}

void ARTUnit::HideContactGhost()
{
	if (ContactGhost)
	{
		ContactGhost->SetVisibility(false, false);
	}
}

void ARTUnit::UpdateContactGhost(const FVector& CellCenterWorld, int32 ContactTurn, int32 CurrentTurn)
{
	if (ContactGhost == nullptr)
	{
		return;
	}

	const float Opacity = GhostOpacityForContact(ContactTurn, CurrentTurn);
	if (Opacity <= 0.0f)
	{
		HideContactGhost();
		return;
	}

	// La mesh/posa arrivano dalla skeletal VIVA del Blueprint (Step 6.1), mai dal C++: un'unita' col solo
	// cilindro segnaposto (#287) non ha nulla da copiare, e la sagoma resta nascosta invece di mostrare
	// un vuoto.
	USkeletalMeshComponent* HeroSkeletal = FindHeroSkeletal();
	if (HeroSkeletal == nullptr || HeroSkeletal->GetSkeletalMeshAsset() == nullptr)
	{
		HideContactGhost();
		return;
	}

	if (ContactGhost->GetSkeletalMeshAsset() != HeroSkeletal->GetSkeletalMeshAsset())
	{
		ContactGhost->SetSkeletalMesh(HeroSkeletal->GetSkeletalMeshAsset());
	}

	// Materiale OPZIONALE (Task 6): se `M_LastContactGhost` non risolve — non ancora creato, o rimosso — la
	// sagoma resta visibile col materiale di DEFAULT della mesh: una sagoma non colorata, mai un crash.
	if (ContactGhostDynMaterial == nullptr && !ContactGhostMaterial.IsNull())
	{
		if (UMaterialInterface* BaseMaterial = ContactGhostMaterial.LoadSynchronous())
		{
			ContactGhostDynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (ContactGhostDynMaterial)
			{
				ContactGhost->SetMaterial(0, ContactGhostDynMaterial);
			}
		}
	}
	if (ContactGhostDynMaterial)
	{
		ContactGhostDynMaterial->SetScalarParameterValue(TEXT("GhostOpacity"), Opacity);
	}

	// Quota dedicata (#983 + Task 6): sopra `RTCellTopZ`, come ogni decoro a terra deve stare. Posizione nel
	// MONDO, non relativa: `ContactGhost` porta `SetUsingAbsoluteLocation`/`Rotation`, quindi non segue mai
	// l'attore, che nel frattempo puo' essersi mosso altrove.
	ContactGhost->SetWorldLocation(CellCenterWorld + FVector(0.f, 0.f, RTLastContactGhostZ));
	ContactGhost->SetWorldRotation(FRotator(0.f, MeshYawOffset, 0.f)); // stessa compensazione della skeletal viva; nessuna freccia di facing per la sagoma
	ContactGhost->SetVisibility(true, false);
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
	// Gli HeroId sono namespaced (`Hero.Gadget`): a schermo serve l'ultimo segmento. Se un giorno l'ID smettesse
	// di avere il punto, questa resta corretta invece di mostrare una stringa vuota.
	int32 Dot = INDEX_NONE;
	if (Full.FindLastChar(TEXT('.'), Dot) && Dot >= 0 && Dot + 1 < Full.Len())
	{
		return Full.RightChop(Dot + 1);
	}
	return Full;
}

FString ARTUnit::DisplayLabel(const FText& InDisplayName, FName InHeroId, const FString& Fallback)
{
	// `IsEmptyOrWhitespace` e non `IsEmpty`: un nome fatto di soli spazi a schermo e' indistinguibile da
	// un'etichetta assente, quindi vale come non dichiarato.
	if (!InDisplayName.IsEmptyOrWhitespace())
	{
		return InDisplayName.ToString();
	}
	return ShortHeroName(InHeroId, Fallback);
}

float ARTUnit::RingLocalZ(float VisualZOffset)
{
	// L'attore sta `VisualZOffset` sopra il piano della cella; l'anello scende della stessa quota e risale
	// del clearance. Sotto un root UNITARIO (#593) la posizione relativa non e' piu' scalata da nessuno,
	// quindi non c'e' niente da compensare — ed e' per questo che il parametro `ParentScaleZ` e' sparito
	// invece di valere sempre 1: un argomento che nessun chiamante puo' variare e' un dato che nessuno legge.
	return -VisualZOffset + RingGroundClearance;
}

float ARTUnit::TeamRingLocalZ(float VisualZOffset)
{
	// Sopra il SelectionRing: nella corona interna vince il colore di SQUADRA, che e' l'informazione
	// permanente; la selezione resta leggibile come cornice esterna, dove il TeamRing non arriva.
	return RingLocalZ(VisualZOffset) + RingStackSeparation;
}

float ARTUnit::SelectionRingLocalZ(float VisualZOffset)
{
	// Alla quota-terra di riferimento. NON si scende sotto: il margine sul disco della cella e' 0.3.
	return RingLocalZ(VisualZOffset);
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

	// Quota dei due anelli a terra: compensa il solo VisualZOffset — sotto un root unitario non c'e' scala
	// da dividere.
	//
	// 🔴 **Le due quote sono DIVERSE dal 2026-08-25, e devono esserlo**: erano lo stesso valore, i due
	// dischi sono concentrici e le loro facce coincidevano — a schermo l'unita' selezionata lampeggiava fra
	// il colore di squadra e quello di selezione. Restano pero' derivate dalla stessa sorgente
	// (`RingLocalZ`), che e' la ragione per cui il valore era uno solo: due chiamate indipendenti si
	// desincronizzano al primo che cambia (code review di #593).
	const float TeamRingZ = TeamRingLocalZ(VisualZOffset);
	const float SelectionRingZ = SelectionRingLocalZ(VisualZOffset);

	// Anello di team: colorato se M_TeamRing c'e', altrimenti resta il colore sul cilindro (fallback).
	// ⚠️ Qui si decide il MATERIALE, non la visibilita': quella la deriva `RefreshComponentVisibility` in
	// fondo, che e' l'unico posto a conoscere anche `bKnownToObserver` e la morte.
	if (TeamRing)
	{
		TeamRing->SetRelativeLocation(FVector(0.f, 0.f, TeamRingZ));
		if (UMaterialInterface* RingBase = TeamRingMaterial.LoadSynchronous())
		{
			RingDynMaterial = UMaterialInstanceDynamic::Create(RingBase, this);
			TeamRing->SetMaterial(0, RingDynMaterial);
			RingDynMaterial->SetVectorParameterValue(TEXT("Color"), TeamColor);
		}
	}

	// Anello di selezione: quota-terra di riferimento (il TeamRing gli sta sopra), colore di selezione.
	// Senza materiale di selezione non compare mai — ed e' il predicato `ShouldShowSelectionRing`, non un
	// `SetVisibility(false)` scritto qui, a dirlo.
	if (SelectionRing)
	{
		SelectionRing->SetRelativeLocation(FVector(0.f, 0.f, SelectionRingZ)); // sotto il TeamRing, non complanare
		if (UMaterialInterface* SelBase = SelectionRingMaterial.LoadSynchronous())
		{
			SelectionRingDynMaterial = UMaterialInstanceDynamic::Create(SelBase, this);
			SelectionRing->SetMaterial(0, SelectionRingDynMaterial);
			SelectionRingDynMaterial->SetVectorParameterValue(TEXT("Color"), SelectionColor);
		}
	}

	// I due materiali appena decisi entrano nello stato: la visibilita' e' una funzione di quello stato, e
	// si ricalcola qui invece di essere assegnata componente per componente qui sopra.
	RefreshComponentVisibility();
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
	const FVector Current = GetActorLocation();

	if ((World - Current).SizeSquared2D() > UE_KINDA_SMALL_NUMBER)
	{
		// Direzione dello spostamento: la dichiara `GetVelocity()` agli AnimBP.
		//
		// ⚠️ **Fuori dal ramo del facing, e non dentro.** Un'unita' con `bFaceMovementDirection = false` si
		// muove lo stesso, e l'animazione di corsa deve sapere dove sta andando anche quando la mesh non si
		// volta: calcolarla dentro l'`if` la lascerebbe ferma alla direzione precedente.
		LastVisualDirection = FVector(World.X - Current.X, World.Y - Current.Y, 0.f).GetSafeNormal();

		// Facing opzionale: orienta l'unita' verso la direzione di spostamento (solo presentazione, solo yaw).
		if (bFaceMovementDirection)
		{
			SetActorRotation(FRotator(0.f, URTPlaybackLibrary::DirectionYaw(Current, World), 0.f));
		}
	}

	SetActorLocation(World); // solo presentazione: lo stato logico (Cell) resta invariato
}

FVector ARTUnit::GetVelocity() const
{
	// `AActor::GetVelocity()` legge il movement component, e questa unita' non ne ha: il playback la sposta
	// per interpolazione, quindi la velocita' vera sarebbe SEMPRE zero. Gli AnimBP dei pack Paragon leggono
	// proprio quel valore per scegliere idle/corsa e la direzione del blendspace: agganciati cosi' com'e',
	// resterebbero fermi in idle senza un errore, senza un warning e senza un log.
	//
	// La velocita' si DICHIARA dallo stato che il TurnManager gia' scrive, e non si stima con `DeltaTime`:
	// il modulo e' un parametro di presentazione, la direzione e' l'ultimo spostamento vero.
	//
	// ⚠️ Solo presentazione (invariante #1): nessuna regola legge questo valore.
	return bIsMovingVisually ? LastVisualDirection * VisualRunSpeed : FVector::ZeroVector;
}

void ARTUnit::OnSelected()
{
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(BaseMeshScale * 1.15f); // cilindro segnaposto: ingrandisce del 15%
	}

	// La selezione muta il proprio FLAG e chiede una refresh: non tocca componenti. Il riscontro visibile
	// resta l'anello a terra — che compare anche sugli skeletal, dove il cilindro e' nascosto — ma la
	// decisione «si vede o no» la prende `RefreshComponentVisibility`, che conosce anche la conoscenza e il
	// materiale. Accenderlo da qui rimetterebbe uno scrittore in piu' sullo stesso componente.
	bSelected = true;
	RefreshComponentVisibility();
}

void ARTUnit::OnDeselected()
{
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(BaseMeshScale);
	}
	bSelected = false;
	RefreshComponentVisibility();
}

bool ARTUnit::ApplyStatus(FGameplayTag Tag, int32 Turns)
{
	// Catalogo terreni §2: `Burning` e' "rimosso da `Wet`". La regola sta QUI e non nel chiamante, cosi'
	// vale per ogni sorgente di bagnato — acqua bassa, `Phase.PressureJet`, `CircularTide` — invece che nel
	// punto che si e' ricordato di scriverla. L'ordine e' voluto: si spegne anche se il Wet arriva insieme.
	//
	// ⚠️ Lo spegnimento si RESTITUISCE (`#1314`): qui il TurnLog non c'e', e senza dirlo a chi chiama
	// questo momento resta muto — che e' precisamente il difetto misurato.
	bool bExtinguished = false;
	if (Tag == TAG_Status_Wet)
	{
		bExtinguished = StatusTurns.Remove(TAG_Status_Burning) > 0;
	}

	if (Turns == PersistentWhileOnCell)
	{
		CellBoundStatuses.Add(Tag); // scade quando l'unita' lascia la cella, non a tempo
		return bExtinguished;
	}
	if (Turns <= 0)
	{
		return bExtinguished;
	}
	int32& Current = StatusTurns.FindOrAdd(Tag);
	Current = FMath::Max(Current, Turns); // riapplicare non accorcia una durata piu' lunga
	return bExtinguished;
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
	// `Action.Cleanse` purifica lo stato, non una delle sue sorgenti: chi e' bagnato dall'acqua E da Phase
	// esce pulito da entrambe. Restare bagnati stando nell'acqua e' comunque il comportamento del turno
	// dopo, perche' la cella lo riapplica all'ingresso successivo.
	StatusTurns.Remove(Tag);
	CellBoundStatuses.Remove(Tag);
	return true;
}

namespace
{
	/**
	 * Ordina i tag per NOME, e non e' cosmesi (#1077).
	 *
	 * 🔴 I due contenitori da cui questi tag escono sono una `TSet` e una `TMap`: il loro ordine di
	 * iterazione non e' una proprieta' del gioco. Consegnarlo cosi' com'e' al TurnLog farebbe dipendere
	 * l'ORDINE DELLE VOCI dall'implementazione del contenitore, quindi l'hash del turno cambierebbe fra
	 * due esecuzioni identiche — l'invariante «niente dipendenza dall'ordine di `TMap`/`TSet`» esiste per
	 * questo, e `HashTurnLogOrdered` esiste per renderlo visibile quando succede.
	 */
	void OrdinaPerNome(TArray<FGameplayTag>& Tags)
	{
		// 🔴 **`FName::Compare` e NON `FString::operator<`**, ed e' una correzione di code review: la prima
		// stesura rifaceva, in un'altra forma, il difetto che `RTTurnLogLibrary.cpp` documenta a proprie
		// spese — `FString::UEOpLessThan` e' `FPlatformString::Stricmp(...) < 0`, quindi **non e' un ordine
		// totale** sui byte. Due tag che differiscono solo per il caso pareggerebbero in entrambi i versi,
		// resterebbero a pari merito, e `TArray::Sort` — che non e' stabile — deciderebbe secondo l'ordine
		// di iterazione del contenitore: esattamente il non-determinismo che questa funzione esiste per
		// togliere. In piu' `Compare` non alloca due `FString` per confronto.
		Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
		{
			return A.GetTagName().Compare(B.GetTagName()) < 0;
		});
	}
}

TArray<FGameplayTag> ARTUnit::RevokeCellBoundStatusesNotIn(const TSet<FGameplayTag>& Sustained)
{
	TArray<FGameplayTag> Revocati;
	for (auto It = CellBoundStatuses.CreateIterator(); It; ++It)
	{
		if (!Sustained.Contains(*It))
		{
			Revocati.Add(*It);
			It.RemoveCurrent();
		}
	}
	OrdinaPerNome(Revocati);
	return Revocati;
}

TArray<FGameplayTag> ARTUnit::TickStatuses()
{
	TArray<FGameplayTag> Scaduti;
	for (auto It = StatusTurns.CreateIterator(); It; ++It)
	{
		if (--It.Value() <= 0)
		{
			if (It.Key() == TAG_Status_Marked)
			{
				MarkedByTeam = INDEX_NONE; // scaduto senza essere speso: la provenienza scade con lui
			}
			Scaduti.Add(It.Key());
			It.RemoveCurrent();
		}
	}
	OrdinaPerNome(Scaduti);
	return Scaduti;
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
	// Scatto generico: portata, ricarica e identita' vengono da `Action.Dodge`, non da numeri inventati qui.
	// E' anche cio' che lo rende riconoscibile come mobilita' rapida: il gate e' la FASE dichiarata dal
	// catalogo (`URTCatalogLibrary::IsFastMovement`), non un flag booleano sull'asset.
	const FRTActionDef DodgeDef = URTCatalogLibrary::FindCoreAction(TEXT("Action.Dodge"));
	URTActionData* Scatto = MakeAbility(TEXT("Scatto"), DodgeDef.RangeCells, 0, 0, DodgeDef.CooldownTurns, 0, FGameplayTag(), 0);
	Scatto->Def = DodgeDef;
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

	// ⚠️ Questo elenco è il confine, ed è la ragione per cui esiste
	// `RefactorTactics.Unit.HeroDataCrossesTheBoundary`: `ARTUnit` è una COPIA PER VALORE di `URTHeroData`,
	// quindi ogni campo non nominato qui si perde in silenzio. È già successo due volte —
	// `HeroDisplayName` e `HearingThreshold` erano dichiarati e popolati a catalogo, e non arrivavano.
	// Un campo aggiunto all'eroe senza una riga qui rende rosso quel test il giorno stesso.
	HeroId = Hero->HeroId;
	HeroDisplayName = Hero->DisplayName;
	MaxHealth = Hero->MaxHealth;
	MoveRange = Hero->MovePoints;
	VisionRange = Hero->VisionRange;
	HearingThreshold = Hero->HearingThreshold;
	PushResistance = Hero->PushResistance;
	Affinity = Hero->Affinity;
	Weakness = Hero->Weakness;
	// E14.7 [D-047]: il profilo che il `Brace` armera'. `NAME_None` per chi non ne dichiara uno (Riktor), ed
	// e' un valore legittimo — non un campo dimenticato: significa profilo base, cardinalita' 1.
	ReactionProfileId = Hero->ReactionProfileId;

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

void ARTUnit::EquipLoadout(const TArray<FName>& PieceIds)
{
	for (const FName& PieceId : PieceIds)
	{
		const URTEquipmentData* Piece = URTCatalogLibrary::FindEquipment(PieceId);
		if (Piece == nullptr)
		{
			continue; // un pezzo che non esiste si salta: vedi la nota sull'header
		}

		if (Piece->Slot == ERTEquipmentSlot::WeaponVariant)
		{
			if (!Abilities.IsValidIndex(0) || Abilities[0] == nullptr)
			{
				continue; // nessun attacco base da modificare: niente da fare, non un errore
			}

			// DUPLICA prima di modificare. `Abilities` porta i puntatori del catalogo eroi, non copie:
			// scrivere direttamente su `Abilities[0]` modificherebbe il dato condiviso, e la variante si
			// applicherebbe a ogni unita' che lo condivide — accumulando, perche' `RangeCells` somma.
			Abilities[0] = DuplicateObject<URTActionData>(Abilities[0], this);
			URTCatalogLibrary::EquipWeaponVariant(Abilities[0], Piece);
			continue;
		}

		if (URTActionData* Granted = URTCatalogLibrary::MakeEquipmentAction(Piece, this))
		{
			Abilities.Add(Granted);
		}
	}

	// I due specchi che il resto del motore legge davvero. `ARTTurnManager` prende la portata da
	// `AttackRange`, non dall'azione: senza questa riga un'unita' con la variante applicata continuerebbe a
	// colpire alla portata VECCHIA in partita, mentre ogni test sull'azione la vedrebbe cambiata.
	if (Abilities.IsValidIndex(0) && Abilities[0])
	{
		AttackRange = Abilities[0]->RangeCells;
		AttackPower = Abilities[0]->Power;
	}

	// Il kit e' cresciuto: i gadget e i moduli hanno cooldown propri e servono i loro slot.
	SyncAbilityCooldowns();
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

void ARTUnit::ClearReactionPlan()
{
	PlannedReactionAbility = INDEX_NONE;
	PlannedReactionCondition = FRTDeclaredCondition();
}

bool ARTUnit::SetPlannedReactionCondition(const FRTDeclaredCondition& Condition)
{
	// Togliere la condizione e' sempre legittimo: e' il modo di tornare a «rispondi comunque». Non passa dal
	// validator, e infatti `NAME_None` non e' fra gli id ammessi — non deve esserlo.
	if (!Condition.IsDeclared())
	{
		PlannedReactionCondition = FRTDeclaredCondition();
		return true;
	}

	// Una condizione senza reazione a cui applicarsi resterebbe orfana nel piano, e il prossimo armamento se
	// la ritroverebbe addosso senza averla chiesta.
	if (PlannedReactionAbility == INDEX_NONE)
	{
		return false;
	}

	// O entra intera, o il piano resta com'era: una condizione applicata a meta' e' peggio di nessuna
	// condizione, perche' il giocatore crederebbe di aver ristretto il fuoco.
	if (!URTReactionOpportunityLibrary::IsDeclaredConditionAllowed(Condition))
	{
		return false;
	}

	PlannedReactionCondition = Condition;
	return true;
}

bool ARTUnit::CanUseAbility(int32 Index) const
{
	const URTActionData* Ability = GetAbility(Index);
	return Ability && URTCombatLibrary::IsAbilityUsable(GetAbilityCooldown(Index), Energy, Ability->EnergyCost);
}

void ARTUnit::SelectAbility(int32 Index)
{
	// `INDEX_NONE` DISARMA, ed e' un ingresso legittimo e non un errore da scartare: senza di esso non
	// esisterebbe un modo di tornare allo stato neutro di D-128, e `RMB` non potrebbe uscire da un
	// targeting. Ogni altro indice non valido resta ignorato, com'era.
	if (Index == INDEX_NONE || Abilities.IsValidIndex(Index))
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
