#include "UI/RTScreenHudWidgets.h"

#include "RefactorTactics.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"
#include "Turn/RTTurnManagerAccess.h" // FindTurnManagerInWorld: la ricerca senza l'header dell'orchestratore
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

	// La ricerca vive in `Turn/RTTurnManagerAccess.h` da `#1821`: questo file non chiama **nessun** metodo
	// del manager — lo cerca, lo tiene in un weak pointer e lo passa alla view model — quindi non ha
	// ragione di includerne l'header da 1 856 righe. Il retry qui sotto resta dov'era: la ricerca cambia
	// indirizzo, non momento.
	TurnManager = FindTurnManagerInWorld(World);

	// 🔴 **Il HUD di partita puo' nascere SENZA owning player, e senza di esso non esiste una selezione.**
	// `URTFrontendNavigator::PresentMatchHud` lo crea con `CreateWidget(GameInstance, ...)` dentro
	// `EnterMatch()`, cioe' nel `BeginPlay` del GameMode: se in quel momento il `PlayerController` locale
	// non c'e' ancora, il widget resta senza proprietario **per sempre**.
	//
	// La conseguenza non e' cosmetica: `GetSelectedUnit()` passa da `GetOwningPlayer()`, quindi
	// `URTSelectedUnitPanelWidget::HasSelection()` risponde sempre `false` e il pannello resta `Collapsed`
	// anche con un'unita' selezionata. Header e roster invece funzionano — leggono il `TurnManager` dal
	// mondo e non il controller — ed e' la ragione per cui il sintomo sembra riguardare un widget solo.
	//
	// ⚠️ Difensivo, non correttivo: se il proprietario c'e' gia' questo blocco non fa nulla. Il `Log` scatta
	// una volta sola per widget, e serve a sapere **se** il caso si verifica davvero in partita.
	if (!GetOwningPlayer())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			SetOwningPlayer(PC);
			UE_LOG(LogRT, Log,
				TEXT("Screen HUD: '%s' era senza owning player, agganciato ora. "
					 "Senza, la selezione sarebbe rimasta invisibile al pannello."),
				*GetClass()->GetName());
		}
	}

	// La squadra viene dal controller che POSSIEDE il widget, non da un default: in split-screen o in una
	// futura sessione a due controller, un `PlayerTeamId` costante mostrerebbe a entrambi lo stesso roster.
	if (const ARTPlayerController* PC = Cast<ARTPlayerController>(GetOwningPlayer()))
	{
		PlayerTeamId = ARTPlayerState::TeamIdOf(PC);
	}
}

void URTScreenHudWidgetBase::SetMatchContextForTest(TWeakObjectPtr<ARTTurnManager> InTurnManager, int32 InPlayerTeamId)
{
	TurnManager = InTurnManager;
	PlayerTeamId = InPlayerTeamId;
}

void URTScreenHudWidgetBase::SetSelectedUnitForTest(ARTUnit* InUnit)
{
	SelectedUnitForTest = InUnit;
}

bool URTScreenHudWidgetBase::HasMatchContext() const
{
	return TurnManager.IsValid();
}

const ARTUnit* URTScreenHudWidgetBase::GetSelectedUnit() const
{
	// L'iniezione dei test viene PRIMA, e solo perche' in gioco e' sempre nulla: senza un `ULocalPlayer`
	// — che una run headless non ha — `GetOwningPlayer()` resta nullo e i widget che dipendono dalla
	// selezione non sarebbero verificabili. Vedi `SetSelectedUnitForTest`.
	if (const ARTUnit* Injected = SelectedUnitForTest.Get())
	{
		return Injected;
	}

	const ARTPlayerController* PC = Cast<ARTPlayerController>(GetOwningPlayer());
	return PC ? PC->GetSelectedUnit() : nullptr;
}

const URTIconCatalogData* URTScreenHudWidgetBase::GetIconCatalog() const
{
	// La radice PUO' essere questo stesso widget: `URTTacticalHUDWidget` deriva da questa base, e
	// `GetTypedOuter` cerca fra gli OUTER — non guarda `this`. Senza questo ramo il contenitore sarebbe
	// l'unico widget dell'HUD incapace di leggere il proprio catalogo, che e' il difetto piu' facile da non
	// notare: funziona ovunque tranne dove il dato vive.
	if (const URTTacticalHUDWidget* Self = Cast<URTTacticalHUDWidget>(this))
	{
		return Self->IconCatalog;
	}

	// Un `UUserWidget` innestato nel Designer ha per outer il `UWidgetTree` del padre, il cui outer e' il
	// `UUserWidget` padre: la catena arriva alla radice. Vale anche per un widget creato a runtime con
	// `CreateWidget(this, ...)` da un figlio dell'HUD, perche' l'outer e' allora il chiamante.
	if (const URTTacticalHUDWidget* Root = GetTypedOuter<URTTacticalHUDWidget>())
	{
		return Root->IconCatalog;
	}

	// ⚠️ `nullptr` e' un esito legittimo, non un errore da segnalare qui: il widget puo' vivere fuori
	// dall'HUD (un test, un'anteprima d'editor). Chi consuma passa da `ResolveIcon`, che con catalogo nullo
	// da' il missing-icon e logga la chiave — la diagnostica sta li', in un posto solo.
	return nullptr;
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

	// La regola — `RoundLimit == 0` = «nessun limite», mai `Round 3/0` — vive in una sede sola da #2184:
	// il Canvas di `ARTHUD` ne aveva una seconda copia, e con `rt.HUD.CanvasPanels` attivo i due contatori
	// rendono nello stesso fotogramma, quindi potevano dissentire. Qui resta il solo vestito `FText`, che e'
	// cio' che il binding vuole; la decisione la fa `URTHudViewModel::ComposeRoundCounter`.
	return FText::FromString(URTHudViewModel::ComposeRoundCounter(Header));
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

void URTActionSlotWidget::SetAction(const FRTAbilityCooldownView& InAction, bool bInArmed,
	const URTIconCatalogData* InCatalog)
{
	Action = InAction;
	bArmed = bInArmed;
	ReceivedCatalog = InCatalog;

	// ⚠️ L'evento va per ULTIMO: e' il Blueprint che disegna, e disegna leggendo i tre campi qui sopra. Se
	// partisse prima, un'implementazione che chiama `GetResolvedIcon()` leggerebbe il catalogo del turno
	// PRECEDENTE — un difetto che a schermo somiglia a un ritardo di un frame invece che a un errore.
	OnActionChanged();
}

FRTIconResolution URTActionSlotWidget::GetResolvedIcon() const
{
	// Il consumer e' fisso qui e non arriva dal grafo: `ResolveIcon` lo usa per dire QUALE widget ha chiesto
	// un'icona che non c'era, e sei slot che lo compongono ciascuno per conto proprio possono scriverci sei
	// stringhe diverse — o nessuna. La warning perderebbe l'unica cosa per cui esiste.
	return URTIconLibrary::ResolveIcon(ReceivedCatalog, GetIconId(), TEXT("ActionSlot"));
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
