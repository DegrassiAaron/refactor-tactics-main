#include "UI/RTScreenHudWidgets.h"

#include "Player/RTPlayerController.h"
#include "Turn/RTTurnManager.h"
#include "Unit/RTUnit.h"
#include "UI/RTIconLibrary.h"
#include "Kismet/GameplayStatics.h"

// =====================================================================================================
// Base: il contesto, e nient'altro
// =====================================================================================================

void URTScreenHudWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	AcquireMatchContext();
}

void URTScreenHudWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 🔴 **Il contesto puo' non esserci ancora al `NativeConstruct`, e nel percorso normale non c'e'.**
	// `ARTGameMode::BeginPlay` presenta il HUD (`EnterMatch`, riga 295) **prima** di spawnare il
	// `ARTTurnManager` (riga 337): il widget cerca un actor che non esiste, e senza questo retry resta
	// senza contesto per tutta la partita — «—» al posto di «Round 1/12», con la riga di `ARTHUD` accanto
	// che invece il numero ce l'ha, perche' legge il manager ogni frame.
	//
	// ⚠️ **Non e' un tick di gameplay e non decide nulla**: e' presentazione che si aggancia al proprio
	// dato. Il vincolo del progetto — niente `DeltaTime` per il sequencing competitivo — resta intatto:
	// `InDeltaTime` qui non viene nemmeno letto.
	//
	// ⚠️ La ricerca smette da sola: `IsValid()` diventa vero al primo frame in cui il manager esiste, e da
	// li' in poi questo corpo esce sulla prima riga.
	if (!HasMatchContext())
	{
		AcquireMatchContext();
	}
}

void URTScreenHudWidgetBase::AcquireMatchContext()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TurnManager = Cast<ARTTurnManager>(
		UGameplayStatics::GetActorOfClass(World, ARTTurnManager::StaticClass()));

	// La squadra viene dal controller che POSSIEDE il widget, non da un default: in split-screen o in una
	// futura sessione a due controller, un `PlayerTeamId` costante mostrerebbe a entrambi lo stesso roster.
	if (const ARTPlayerController* PC = Cast<ARTPlayerController>(GetOwningPlayer()))
	{
		PlayerTeamId = PC->PlayerTeamId;
	}
}

void URTScreenHudWidgetBase::SetMatchContextForTest(ARTTurnManager* InTurnManager, int32 InPlayerTeamId)
{
	TurnManager = InTurnManager;
	PlayerTeamId = InPlayerTeamId;
}

bool URTScreenHudWidgetBase::HasMatchContext() const
{
	return TurnManager.IsValid();
}

const ARTUnit* URTScreenHudWidgetBase::GetSelectedUnit() const
{
	const ARTPlayerController* PC = Cast<ARTPlayerController>(GetOwningPlayer());
	return PC ? PC->GetSelectedUnit() : nullptr;
}

// =====================================================================================================
// Turn header
// =====================================================================================================

FRTMatchHeaderView URTTurnHeaderWidget::GetHeader() const
{
	// `BuildMatchHeader` gestisce gia' il manager nullo con una vista neutra: qui non serve una seconda
	// guardia, e averla significherebbe due posti che decidono cosa vuol dire «nessun contesto».
	return URTHudViewModel::BuildMatchHeader(GetTurnManager());
}

FText URTTurnHeaderWidget::GetRoundCounterText() const
{
	const FRTMatchHeaderView Header = GetHeader();

	if (!HasMatchContext())
	{
		return FText::FromString(TEXT("—"));
	}

	// `RoundLimit == 0` = «nessun limite dichiarato», che NON e' «su zero». Una partita senza formato non e'
	// una partita gia' scaduta, e un binding ingenuo stamperebbe `Round 3/0`.
	if (Header.RoundLimit > 0)
	{
		return FText::FromString(FString::Printf(TEXT("Round %d/%d"), Header.Round, Header.RoundLimit));
	}
	return FText::FromString(FString::Printf(TEXT("Round %d"), Header.Round));
}

// =====================================================================================================
// Team roster
// =====================================================================================================

TArray<FRTUnitCardView> URTTeamRosterWidget::GetRoster() const
{
	TArray<FRTUnitCardView> Empty;

	const UWorld* World = GetWorld();
	if (!World)
	{
		return Empty;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(const_cast<UWorld*>(World), ARTUnit::StaticClass(), Found);

	TArray<ARTUnit*> Units;
	Units.Reserve(Found.Num());
	for (AActor* Actor : Found)
	{
		if (ARTUnit* Unit = Cast<ARTUnit>(Actor))
		{
			Units.Add(Unit);
		}
	}

	// ⚠️ L'ordine di `GetAllActorsOfClass` non e' dichiarato, e un roster che cambia ordine fra due frame e'
	// illeggibile. Si ordina per un criterio STABILE e indipendente dal mondo: l'id dell'eroe. Non e' una
	// preferenza estetica — e' la stessa disciplina per cui il resolver non si affida all'ordine di `TMap`.
	Units.Sort([](const ARTUnit& A, const ARTUnit& B)
	{
		return A.HeroId.LexicalLess(B.HeroId);
	});

	return URTHudViewModel::BuildTeamRoster(Units, GetPlayerTeamId());
}

// =====================================================================================================
// Selected unit panel
// =====================================================================================================

bool URTSelectedUnitPanelWidget::HasSelection() const
{
	return GetSelectedUnit() != nullptr;
}

FRTUnitCardView URTSelectedUnitPanelWidget::GetCard() const
{
	return URTHudViewModel::BuildUnitCard(GetSelectedUnit(), GetPlayerTeamId());
}

FRTUnitSlotsView URTSelectedUnitPanelWidget::GetSlots() const
{
	return URTHudViewModel::BuildUnitSlots(GetSelectedUnit());
}

// =====================================================================================================
// Action dock
// =====================================================================================================

TArray<FRTAbilityCooldownView> URTActionDockWidget::GetActions() const
{
	return URTHudViewModel::BuildAbilityCooldowns(GetSelectedUnit());
}

int32 URTActionDockWidget::GetArmedActionIndex() const
{
	const ARTUnit* Unit = GetSelectedUnit();

	// `INDEX_NONE` anche senza selezione, e le due cose coincidono di proposito: «nessuna unita'» e «nessuna
	// azione armata» danno lo stesso dock spento, che e' cio' che il giocatore deve vedere in entrambi i casi.
	return Unit ? Unit->SelectedAbilityIndex : INDEX_NONE;
}

// =====================================================================================================
// Action slot
// =====================================================================================================

void URTActionSlotWidget::SetAction(const FRTAbilityCooldownView& InAction, bool bInArmed)
{
	Action = InAction;
	bArmed = bInArmed;
	OnActionChanged();
}

FName URTActionSlotWidget::GetIconId() const
{
	// `Action.ActionId` e' il percorso semantico che il gioco usa gia': `MakeIconId` ne fa la chiave. Un'azione
	// senza `ActionId` — quelle create in codice prima del motore azioni — non ha icona, e restituire `None`
	// e' meglio di comporre `UI.Icon.` a vuoto: la risoluzione direbbe «chiave sconosciuta» nominando una
	// chiave che nessuno ha mai dichiarato.
	if (Action.ActionId.IsNone())
	{
		return NAME_None;
	}
	return URTIconLibrary::MakeIconId(Action.ActionId);
}
