#include "UI/RTScreenHudWidgets.h"

#include "RefactorTactics.h"
#include "Player/RTPlayerController.h"
#include "Player/RTPlayerState.h"
#include "Turn/RTTurnManagerAccess.h" // FindTurnManagerInWorld: la ricerca senza l'header dell'orchestratore
// 🔴 **`Turn/RTTurnManager.h` NON serve piu', e la storia va tenuta perche' il difetto era vero.**
// #2257 l'ha rimesso il 2026-09-04 con questa diagnosi, esatta: `TurnManager` e' un
// `TWeakObjectPtr<ARTTurnManager>`, assegnarci un puntatore nudo vuole la definizione — il compilatore deve
// sapere che deriva da `UObject` — e il file **compilava solo grazie alla UNITY BUILD**, che il tipo glielo
// offriva di rimbalzo da un vicino di blob. Difetto latente reale, trovato per due strade indipendenti lo
// stesso giorno: #2257 dall'Adaptive Build che ha ricomposto i blob, #2184 toccando il file per altro.
//
// ✅ **Ma la premessa e' caduta: qui non si ASSEGNA piu' un puntatore nudo.** Dal #2184
// `FindTurnManagerInWorld` rende un `TWeakObjectPtr<ARTTurnManager>` costruito dentro
// `RTTurnManagerAccess.cpp`, l'unico posto che paga l'header, e `SetMatchContextForTest` ne prende uno: la
// copia weak → weak non tocca `T` e non chiede la definizione. Restava solo il costo.
//
// ⚠️ **Toglierlo non e' un ripristino di comodo: e' cio' che rende VERO il distacco di #1821.** Con
// l'include, la prova strutturale di quella issue — «un file che non include piu' l'header non ne aveva
// bisogno» — resta soddisfatta a vuoto. Verificato compilando questo file FUORI dal blob unity.
#include "Unit/RTUnit.h"
#include "UI/RTIconLibrary.h"
#include "UI/RTHUD.h" // ComposeAbilityLine: lo slot la INOLTRA, non ne scrive una seconda
#include "UI/RTReactionWindowViewModel.h" // il view model si INTERROGA: qui non si costruisce e non si lega
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetTree.h" // ComposeMountReport cammina l'albero COSTRUITO, non quello progettato

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
	if (ARTPlayerController* PC = Cast<ARTPlayerController>(GetOwningPlayer()))
	{
		PlayerTeamId = ARTPlayerState::TeamIdOf(PC);

		// ⚠️ **Dallo STESSO controller da cui viene la squadra, e non dal primo del mondo** (CP 14.6, `#166`).
		// La finestra e' una domanda posta a **un** giocatore: risolverla su `GetFirstPlayerController()`
		// sarebbe corretto oggi — un umano solo, [D-155] — e sbagliato al primo split-screen, mostrando a
		// entrambi la finestra di uno.
		//
		// ⚠️ `GetReactionWindowViewModel()` COSTRUISCE il view model se non c'e' ancora, e va bene: e'
		// l'oggetto del controller, non una copia di questo widget, e chi lo lega al manager resta
		// `ARTGameMode::HookReactionWindow`. Qui si prende un riferimento, non si accende niente.
		ReactionWindow = PC->GetReactionWindowViewModel();
	}
}

TArray<ARTUnit*> URTScreenHudWidgetBase::GatherUnitsInWorld() const
{
	TArray<ARTUnit*> Units;

	const UWorld* World = GetWorld();
	if (!World)
	{
		return Units;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(const_cast<UWorld*>(World), ARTUnit::StaticClass(), Found);

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
	//
	// ⚠️ **Sta qui e non nel roster** da `#2744`: anche la scoperta delle squadre legge questo elenco, e due
	// ordini diversi per lo stesso mondo renderebbero l'ordine delle liste dipendente da chi ha chiesto.
	Units.Sort([](const ARTUnit& A, const ARTUnit& B)
	{
		return A.HeroId.LexicalLess(B.HeroId);
	});

	return Units;
}

TArray<int32> URTScreenHudWidgetBase::ResolveObserverTeamIds() const
{
	// Il puntatore si PASSA: `IsUnattendedSession()` e' inline nell'header dell'orchestratore, e leggerla qui
	// riporterebbe dentro questo file la dipendenza che `#2257` ha tolto. Vedi la dichiarazione nell'header.
	return URTHudViewModel::ResolveObserverTeamIds(GetTurnManager(), GetPlayerTeamId(), GatherUnitsInWorld());
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

void URTScreenHudWidgetBase::SetInspectedUnitForTest(ARTUnit* InUnit)
{
	InspectedUnitForTest = InUnit;
}

void URTScreenHudWidgetBase::SetReactionWindowForTest(URTReactionWindowViewModel* InViewModel)
{
	ReactionWindow = InViewModel;
}

URTReactionWindowViewModel* URTScreenHudWidgetBase::GetReactionWindow() const
{
	// ⚠️ **Nessun ripiego su `GetFirstPlayerController()` qui.** `AcquireMatchContext` risolve dal
	// proprietario e basta: un ripiego renderebbe questo accessore l'unico posto dell'HUD in cui «la mia
	// finestra» significa «la finestra di qualcuno», e il difetto si vedrebbe solo con due controller.
	return ReactionWindow.Get();
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

const ARTUnit* URTScreenHudWidgetBase::GetInspectedUnit() const
{
	// Stessa forma della sorella, e per la stessa ragione headless: vedi `SetInspectedUnitForTest`.
	if (const ARTUnit* Injected = InspectedUnitForTest.Get())
	{
		return Injected;
	}

	const ARTPlayerController* PC = Cast<ARTPlayerController>(GetOwningPlayer());
	return PC ? PC->GetInspectedUnit() : nullptr;
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

TArray<FRTPlayerEventLineView> URTPlayerEventLogWidget::GetFeed() const
{
	// ⚠️ **Il manager si passa, non si dereferenzia qui.** Leggere `GetTurnLog()` da questo file
	// pretenderebbe il tipo completo di `ARTTurnManager`, e `#2257` ne ha tolto l'header di proposito:
	// l'overload del view model paga la dipendenza dove gia' esiste. Il manager nullo lo gestisce li'.
	// ⛔ **`GetPlayerTeamId()` e non un letterale**, ed e' la ragione per cui [D-242] ha centralizzato la
	// domanda «di chi e' la vista?»: le copie divergenti che c'erano prima comprendevano un `PlayerTeamId = 0`
	// scritto a mano che alimentava quattro filtri di privacy.
	//
	// ⚠️ **L'insieme e non il singolo** (`#2744`): in sessione presidiata contiene la sola `PlayerTeamId` e
	// questa riga risponde come rispondeva; in autobattle contiene le squadre in campo, e la decisione
	// resta una per voce dentro `Project` — non due proiezioni concatenate, che produrrebbero doppioni.
	return URTHudViewModel::BuildPlayerEventFeed(GetTurnManager(), ResolveObserverTeamIds());
}

TArray<FString> URTPlayerEventLogWidget::DescribeFeedState() const
{
	return URTHudViewModel::DescribeFeedState(GetTurnManager(), ResolveObserverTeamIds());
}


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
	return URTHudViewModel::BuildTeamRoster(GatherUnitsInWorld(), GetPlayerTeamId());
}

TArray<FRTUnitCardView> URTTeamRosterWidget::GetOpposingRoster() const
{
	const TArray<ARTUnit*> Units = GatherUnitsInWorld();
	const int32 OwnTeamId = GetPlayerTeamId();

	TArray<FRTUnitCardView> Roster;
	for (const int32 TeamId : ResolveObserverTeamIds())
	{
		// La propria squadra e' gia' in `GetRoster()`: qui si aggiungono le ALTRE. In sessione presidiata
		// l'insieme contiene solo la propria, quindi questo ciclo non aggiunge niente — ed e' cosi' che la
		// regola «vuoto quando qualcuno gioca» esiste senza essere scritta una seconda volta.
		if (TeamId == OwnTeamId)
		{
			continue;
		}

		// `BuildTeamRoster` filtra per `TeamId ==`: due squadre diverse danno insiemi DISGIUNTI, quindi
		// `Append` non puo' duplicare. Con un filtro che autorizza per voce — il feed — questa stessa forma
		// sarebbe sbagliata.
		Roster.Append(URTHudViewModel::BuildTeamRoster(Units, TeamId));
	}
	return Roster;
}

// =====================================================================================================
// Selected unit panel
// =====================================================================================================

bool URTSelectedUnitPanelWidget::HasSelection() const
{
	return GetSelectedUnit() != nullptr;
}

bool URTSelectedUnitPanelWidget::HasSubject() const
{
	// Il pannello ha qualcosa da mostrare se comanda un'unita' **oppure** ne sta guardando una.
	return GetSubject() != nullptr;
}

const ARTUnit* URTSelectedUnitPanelWidget::GetSubject() const
{
	// 🔑 **Il comando VINCE sull'ispezione, e l'ordine e' la regola.** Se l'ispezione vincesse, guardare un
	// nemico nasconderebbe l'unita' che stai comandando — cioe' ispezionare costerebbe qualcosa, che e'
	// esattamente cio' che la decisione del 2026-09-11 ha escluso dicendo *«convive»*.
	if (const ARTUnit* Commanded = GetSelectedUnit())
	{
		return Commanded;
	}
	return GetInspectedUnit();
}

FRTUnitCardView URTSelectedUnitPanelWidget::GetCard() const
{
	// La carta segue il soggetto, comandato o ispezionato: identita', salute e scudo sono cio' che
	// `ARTHUD::ShouldDrawUnitOverlay` gia' autorizza sopra la testa di un'unita' osservata. `bIsAlly` lo
	// deriva `BuildUnitCard` dalla squadra, quindi chi disegna sa gia' di chi sta guardando la carta.
	return URTHudViewModel::BuildUnitCard(GetSubject(), GetPlayerTeamId());
}

FRTUnitSlotsView URTSelectedUnitPanelWidget::GetSlots() const
{
	// 🔴 **Gli slot seguono il COMANDO, non il soggetto — e questa e' la barriera di privacy.**
	//
	// `FRTUnitSlotsView` e' `{ Movement, Main, Reaction }`: il **piano del turno**. Costruirlo per un'unita'
	// ispezionata significherebbe consegnare al giocatore il piano avversario, ed e' la ragione per cui
	// `ARTPlayerController` tiene `InspectedUnit` separato da `SelectedActor` invece di riusarne uno solo.
	//
	// ⛔ **E non si costruisce-e-poi-nasconde**: per un soggetto non comandato `BuildUnitSlots` **non viene
	// chiamata**. Il dato non lascia il core, quindi non c'e' niente da filtrare a valle e nessun filtro da
	// dimenticare. Cio' che torna e' il default, con `bAuthorized` falso.
	//
	// ⚠️ **Chi disegna deve distinguere `bAuthorized == false` da un piano vuoto**: un'area slot mostrata
	// vuota per un'avversaria direbbe *«non ha pianificato»*, che e' una lettura del suo piano. `#2757` lo
	// vieta gia' in forma piu' forte — *«nessun conteggio o metadato da cui dedurre che un dato privato
	// esiste»*.
	if (const ARTUnit* Commanded = GetSelectedUnit())
	{
		return URTHudViewModel::BuildUnitSlots(Commanded);
	}
	return FRTUnitSlotsView{};
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
	// ── L'icona si risolve QUI, una volta per cambio azione, e non nel getter.
	//
	// 🔴 `ResolveIcon` logga le chiavi che non trova, ed e' cio' per cui esiste. Chiamarla da un property
	// binding la valuta a ogni frame: la seduta del 2026-09-11 ha prodotto 16 388 righe per quattro chiavi.
	// Il docstring di `GetResolvedIcon` prescriveva gia' «un evento, una volta per cambio azione» — questa
	// riga e' quella prescrizione resa vera, invece che affidata a chi scrive il grafo.
	CachedResolvedIcon = URTIconLibrary::ResolveIcon(ReceivedCatalog, GetIconId(), TEXT("ActionSlot"));

	OnActionChanged();
}

void URTActionSlotWidget::SetArmingControllerForTest(ARTPlayerController* InController)
{
	ArmingControllerForTest = InController;
}

ARTPlayerController* URTActionSlotWidget::ResolveArmingController() const
{
	// L'iniezione dei test viene PRIMA, e solo perche' in gioco e' sempre nulla: senza un `ULocalPlayer`
	// — che una run headless non ha — `GetOwningPlayer()` resta nullo e il click non sarebbe verificabile
	// se non aprendo l'Editor. E' la stessa forma, con la stessa ragione misurata, di
	// `URTScreenHudWidgetBase::GetSelectedUnit()`.
	if (ARTPlayerController* Iniettato = ArmingControllerForTest.Get())
	{
		return Iniettato;
	}

	return Cast<ARTPlayerController>(GetOwningPlayer());
}

void URTActionSlotWidget::Activate()
{
	// ⛔ **Uno slot MAI assegnato porta `INDEX_NONE`, e `ArmKitAbility(INDEX_NONE)` DISARMA.** Senza questa
	// guardia un riquadro vuoto — o sopravvissuto alla ricostruzione della lista — spegnerebbe l'azione
	// armata da un altro. E' la guardia di `URTFastDecisionOptionWidget::Choose()` sul proprio proprietario,
	// tradotta sul dato che qui fa le veci del legame.
	//
	// ⚠️ Non e' un controllo di DISPONIBILITA': una posizione di kit vuota porta comunque il proprio indice
	// (`Cooldowns[i].AbilityIndex == i`, `#2987`) e passa di qui. A rifiutarla e' il core.
	if (Action.AbilityIndex == INDEX_NONE)
	{
		return;
	}

	// 🔑 **L'indice, non l'azione, e la stessa porta del tasto.** `ArmKitAbility` prende un `int32` e delega
	// a `SelectAbilityForCurrent`: cooldown, slot reazione, self-target e input bloccato restano decisi in
	// un posto solo, e un click non puo' aggirare un controllo che il tasto rispetta.
	if (ARTPlayerController* PC = ResolveArmingController())
	{
		PC->ArmKitAbility(Action.AbilityIndex);
	}
}

FRTIconResolution URTActionSlotWidget::GetResolvedIcon() const
{
	// ⚠️ **Rende la cache e non ricalcola**: e' sicuro chiamarla da un binding, che e' esattamente cio' che
	// il Blueprint fa. La risoluzione — e il suo log — avvengono in `SetAction`.
	return CachedResolvedIcon;
}

FName URTActionSlotWidget::GetIconId() const
{
	// Un'azione senza `ActionId` — quelle create in codice prima del motore azioni — non ha icona, e
	// restituire `None` e' meglio di comporre `UI.Icon.` a vuoto: la risoluzione direbbe «chiave sconosciuta»
	// nominando una chiave che nessuno ha mai dichiarato.
	if (Action.ActionId.IsNone())
	{
		return NAME_None;
	}

	// 🔑 **Le due chiavi arrivano dalla VISTA, non si compongono qui.** E' la disciplina gia' dichiarata per
	// i badge di stato (`RTHudViewModel.h`): *«porta l'`IconId` e non lascia che sia chi disegna a comporlo»*,
	// perche' comporla nel widget sarebbe una seconda verita' sulla stessa regola. Qui lo slot **sceglie**
	// fra due chiavi gia' derivate, e la derivazione resta in `URTIconLibrary`.
	// ⚠️ **La vista puo' non portarla**: una `FRTAbilityCooldownView` costruita fuori da
	// `BuildAbilityCooldowns` — un test, un grafo — avrebbe `IconId` vuoto, e leggere solo quel campo
	// toglierebbe l'icona a chiunque non passi dal costruttore canonico. La chiave PREFERITA dipende dal solo
	// `ActionId`, quindi si ricava lo stesso; e' il RIPIEGO che richiede il `Def`, e quello resta nella vista.
	const FName Preferred = Action.IconId.IsNone()
		? URTIconLibrary::MakeActionIconId(Action.ActionId)
		: Action.IconId;
	if (Preferred.IsNone())
	{
		return NAME_None;
	}

	// ── Ripiego, finche' l'asset proprio non e' disegnato: l'icona della core da cui l'azione deriva.
	//
	// ⚠️ **Si CHIEDE al catalogo con una query pura, e non si prova `ResolveIcon`**: quella logga, e provarla
	// emetterebbe una warning ogni volta che il ripiego funziona — rumore al posto della diagnostica.
	// ⛔ **`ReceivedCatalog` assente non e' «nessuna icona»**: e' un widget nato prima del catalogo, e allora
	// si torna la preferita — sara' `ResolveIcon` a dire che non c'e' nulla da risolvere, nominando il consumer.
	if (ReceivedCatalog != nullptr
		&& !URTIconLibrary::CatalogHasIcon(ReceivedCatalog, Preferred)
		&& !Action.FallbackIconId.IsNone())
	{
		return Action.FallbackIconId;
	}

	return Preferred;
}

FText URTActionSlotWidget::GetActionLine() const
{
	// Un inoltro, e la riga sopra e' il contratto: comporre qui sarebbe il secondo produttore della stessa
	// stringa. `ComposeAbilityLine` sa gia' che il numero e' 1-based perche' e' il TASTO che il giocatore
	// preme, e non l'indice del kit — e il dock mostra solo il kit numerato (`BuildAbilityCooldowns`), quindi
	// quel numero e' davvero il tasto che arma questo slot. I cinque generici — `G` `B` `C` `X` `Z` di
	// `ARTPlayerController::GenericHotkeys` — non passano da qui e non hanno bisogno di un ramo.
	return FText::FromString(ARTHUD::ComposeAbilityLine(Action, bArmed).Text);
}

// =====================================================================================================
// Fast decision — la finestra di reazione (CP 14.6, #166)
//
// ⚠️ **Quattro corpi che INOLTRANO, e nessuno che decide.** E' il punto: se qui comparisse un ramo che
// sceglie, la DoD di questo checkpoint — «nessuna logica di gioco nel widget» — sarebbe falsa nel file che
// esiste per renderla vera.
// =====================================================================================================

bool URTFastDecisionWidget::IsWindowOpen() const
{
	const URTReactionWindowViewModel* ViewModel = GetReactionWindow();
	return ViewModel && ViewModel->IsWindowOpen();
}

FRTReactionWindowView URTFastDecisionWidget::GetWindow() const
{
	// Senza view model la vista e' ai default, non «vuota da nascondere in Blueprint»: e' la stessa forma che
	// `FilterWindowForTeam` da' a un avversario, e tenerne una sola significa che il widget non ha un ramo in
	// cui distinguere «non ho il contesto» da «non devo sapere».
	const URTReactionWindowViewModel* ViewModel = GetReactionWindow();
	return ViewModel ? ViewModel->GetWindow() : FRTReactionWindowView();
}

float URTFastDecisionWidget::GetRemainingSeconds() const
{
	// `-1.f` e non `0.f`, come il view model e come `FRTMatchHeaderView::PlanningSecondsRemaining`: zero
	// direbbe «scaduta adesso», che e' un'altra cosa da «non ce n'e' una».
	const URTReactionWindowViewModel* ViewModel = GetReactionWindow();
	return ViewModel ? ViewModel->GetRemainingSeconds() : -1.f;
}

void URTFastDecisionWidget::ChooseOption(int32 OptionIndex)
{
	URTReactionWindowViewModel* ViewModel = GetReactionWindow();
	if (!ViewModel)
	{
		return;
	}

	// 🔑 **Si rilegge la vista invece di fidarsi di quella con cui il bottone e' stato disegnato.**
	// `GetWindow()` rende i default appena l'identita' della finestra cambia, quindi un click su un bottone
	// della finestra PRECEDENTE trova `Options` vuoto e cade nel ramo qui sotto. Senza questa rilettura
	// l'indice verrebbe risolto su un elenco che il gioco non sta piu' offrendo.
	const FRTReactionWindowView Window = ViewModel->GetWindow();
	if (!Window.Options.IsValidIndex(OptionIndex))
	{
		// ⚠️ **`Warning` e non `Error`, e non fa nulla.** In una finestra da 3,0 s l'elenco puo' cambiare fra
		// il disegno e il click: e' un ritardo, non un difetto del chiamante. Ma va **visto**, perche' un
		// bottone che non risponde e' il sintomo piu' difficile da diagnosticare a schermo.
		UE_LOG(LogRT, Warning,
			TEXT("[RT] FastDecision: opzione %d fuori range (%d disponibili) — nessuna risposta inoltrata. "
				 "La finestra puo' essere cambiata fra il disegno e il click."),
			OptionIndex, Window.Options.Num());
		return;
	}

	// ⛔ **Si spedisce la stringa che il core ha prodotto, e non se ne compone una.** `FIRE:<indice>` e'
	// un FORMATO con un solo produttore (`URTReactionOpportunityLibrary::FireResponse`); comporlo qui ne
	// creerebbe un secondo, fuori dai test che presidiano il primo.
	ViewModel->SubmitResponse(Window.Options[OptionIndex].Response);
}

int32 URTFastDecisionWidget::GetOptionCount() const
{
	return GetWindow().Options.Num();
}

URTFastDecisionOptionWidget* URTFastDecisionWidget::MakeOptionWidget(
	TSubclassOf<URTFastDecisionOptionWidget> OptionClass, int32 OptionIndex)
{
	if (!OptionClass)
	{
		UE_LOG(LogRT, Warning,
			TEXT("[RT] FastDecision: nessuna classe per il bottone dell'opzione %d — nessun widget creato."),
			OptionIndex);
		return nullptr;
	}

	// 🔑 **Si rilegge la vista invece di fidarsi di quella con cui il grafo ha iniziato il ciclo.**
	// `GetWindow()` rende i default appena l'identita' cambia, quindi un `ForEach` sopravvissuto alla
	// chiusura della finestra trova `Options` vuoto e cade nel ramo qui sotto invece di costruire bottoni
	// per una domanda che non c'e' piu'.
	const FRTReactionWindowView Window = GetWindow();
	if (!Window.Options.IsValidIndex(OptionIndex))
	{
		UE_LOG(LogRT, Warning,
			TEXT("[RT] FastDecision: opzione %d fuori range (%d disponibili) — nessun bottone creato."),
			OptionIndex, Window.Options.Num());
		return nullptr;
	}

	// 🔴 **NON `CreateWidget(this, ...)`, e la ragione e' misurata.** Quella forma pretende che il
	// genitore abbia un `WidgetTree` — `UUserWidget.cpp:2708`, `ensure(ParentUserWidget && ...->WidgetTree)`
	// — e un'istanza C++ costruita con `NewObject` non ce l'ha: l'albero arriva dalla classe generata dal
	// Blueprint. In partita l'ensure non scatta; in una run headless si', e la funzione sarebbe verificabile
	// solo aprendo l'Editor. Trovato dal test, non previsto.
	//
	// ⚠️ **Il proprietario resta quello giusto quando c'e'**: il bottone appartiene al giocatore che
	// possiede la finestra, e solo in sua assenza si ripiega sul mondo. Il ripiego non e' una scorciatoia di
	// test — e' anche il caso di un HUD creato prima che il `PlayerController` locale esista, che
	// `AcquireMatchContext` documenta come percorso normale per qualche frame.
	URTFastDecisionOptionWidget* Bottone = nullptr;
	if (APlayerController* Proprietario = GetOwningPlayer())
	{
		Bottone = CreateWidget<URTFastDecisionOptionWidget>(Proprietario, OptionClass);
	}
	else if (UWorld* Mondo = GetWorld())
	{
		Bottone = CreateWidget<URTFastDecisionOptionWidget>(Mondo, OptionClass);
	}
	if (!Bottone)
	{
		return nullptr;
	}

	// ⛔ **Qui, e non nel grafo, si decide quale opzione e' la SICURA.** `SafeResponse` la NOMINA; nel
	// `Brace` si chiama `Hold Ground`, non `HOLD`. Un grafo che cercasse la parola sarebbe corretto oggi e
	// sbagliato con la prima finestra che non e' un Overwatch.
	const bool bSicura = Window.Options[OptionIndex].Response == Window.SafeResponse;
	Bottone->SetOption(this, Window.Options[OptionIndex], OptionIndex, bSicura);
	return Bottone;
}

ESlateVisibility URTFastDecisionWidget::GetWindowVisibility() const
{
	// 🔑 **`IsWindowOpen()`, e non `GetRemainingSeconds() > 0`.** Sono due orologi: il residuo scorre col
	// `Tick` dell'Actor, il widget disegna col proprio, e il numero tocca lo zero PRIMA che la finestra si
	// chiuda. Un binding costruito sul numero farebbe sparire il prompt mentre il gioco aspetta ancora.
	return IsWindowOpen() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
}

FText URTFastDecisionWidget::GetCountdownText() const
{
	const float Remaining = GetRemainingSeconds();

	// Negativo significa «nessuna finestra» — la convenzione di `FRTMatchHeaderView::PlanningSecondsRemaining`
	// — e non «scaduta». Vuoto, non «0.0»: un numero senza domanda si legge come una domanda scaduta.
	if (Remaining < 0.f)
	{
		return FText::GetEmpty();
	}

	// ⚠️ **Il clamp e' sul BASSO e non serve al numero: serve al segno.** Fra l'ultimo tick dell'orologio
	// autorevole e la chiusura il residuo puo' essere di poco negativo, e `-0.1` a schermo sarebbe l'unico
	// momento in cui questo widget mostra qualcosa che il gioco non ha mai detto.
	FNumberFormattingOptions Format;
	Format.MinimumFractionalDigits = 1;
	Format.MaximumFractionalDigits = 1;
	return FText::AsNumber(FMath::Max(0.f, Remaining), &Format);
}

FText URTFastDecisionWidget::GetPromptText() const
{
	if (!IsWindowOpen())
	{
		return FText::GetEmpty();
	}

	// ⛔ **Niente nome del bersaglio, e la ragione sta nella dichiarazione**: il DTO porta un
	// `TargetSnapshotIndex`, non un id stabile, e risolverlo qui nominerebbe l'unita' sbagliata in ogni
	// partita in cui qualcuno e' gia' caduto. Un'etichetta neutra e' l'unica cosa che questo widget sa.
	return NSLOCTEXT("RefactorTactics", "FastDecisionPrompt", "Reazione");
}

void URTFastDecisionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ⚠️ Non e' un tick di gameplay e non decide nulla: e' presentazione che si accorge di un cambio. Come
	// il retry del contesto sulla base, `InDeltaTime` qui non viene nemmeno letto.
	if (PollWindowChanged())
	{
		// L'evento va per ULTIMO: `PollWindowChanged` ha gia' aggiornato `LastWindowId`, quindi il grafo
		// che ricostruisce legge lo stato della finestra NUOVA.
		OnWindowChanged();
	}
}

bool URTFastDecisionWidget::PollWindowChanged()
{
	// 🔑 **L'identita' si CHIEDE al view model, non si deriva dalla vista.** `DeriveOpportunityId` e' un
	// formato con un solo produttore: ricomporlo qui ne creerebbe un secondo, ed e' esattamente cio' che il
	// view model ha gia' rifiutato di fare per se'.
	const URTReactionWindowViewModel* ViewModel = GetReactionWindow();

	// ⚠️ **Senza view model l'identita' e' VUOTA, non «invariata».** Un HUD che perde il proprio
	// proprietario deve ricostruire — cioe' svuotare i bottoni — non restare con quelli dell'ultima
	// finestra: e' lo stesso caso della finestra che si chiude.
	//
	// ⛔ E si legge l'id solo quando la finestra e' APERTA: `WindowOpportunityId` resta valorizzato anche
	// dopo una scadenza — e' `IsWindowOpen()` a confrontarlo con l'orologio autorevole. Leggerlo senza quel
	// gate terrebbe i bottoni di una finestra che il core ha gia' chiuso.
	const FString Corrente =
		(ViewModel && ViewModel->IsWindowOpen()) ? ViewModel->GetWindowId() : FString();

	if (Corrente == LastWindowId)
	{
		return false;
	}

	LastWindowId = Corrente;
	return true;
}

// =====================================================================================================
// Fast decision option — un bottone, il suo indice, e nient'altro
// =====================================================================================================

void URTFastDecisionOptionWidget::SetOption(URTFastDecisionWidget* InOwner,
	const FRTReactionWindowOptionView& InOption, int32 InIndex, bool bInIsSafe)
{
	Owner = InOwner;
	Option = InOption;
	OptionIndex = InIndex;
	bIsSafeChoice = bInIsSafe;

	// ⚠️ Per ULTIMO, come `URTActionSlotWidget::SetAction`: il Blueprint disegna leggendo i campi qui sopra.
	OnOptionChanged();
}

FText URTFastDecisionOptionWidget::GetOptionLabel() const
{
	// ⚠️ **La stringa di risposta diventa un'etichetta e non torna piu' indietro.** E' l'unico punto in cui
	// `Response` attraversa il confine verso la presentazione, e lo fa come `FText`: da li' non si puo'
	// rimandare al core, perche' `Choose()` spedisce l'INDICE.
	//
	// ⛔ Nessuna traduzione e nessun abbellimento qui: `Response` e' un vocabolario del core
	// (`FIRE:<indice>`, `HOLD`, `Hold Ground`, le maneuver del profilo) e mapparlo su nomi leggibili e'
	// lavoro di contenuto, con una tabella e un owner. Inventarlo qui sarebbe un secondo vocabolario.
	return FText::FromString(Option.Response);
}

void URTFastDecisionOptionWidget::Choose()
{
	// ⛔ **Fail-closed sul proprietario**: un'opzione staccata dalla propria finestra non risponde per
	// nessuno. Il caso non e' teorico — il grafo puo' tenere in vita un bottone oltre la ricostruzione.
	if (URTFastDecisionWidget* Finestra = Owner.Get())
	{
		// L'indice, non la risposta. Il gate della validita' resta in `ChooseOption`, che rilegge la vista.
		Finestra->ChooseOption(OptionIndex);
	}
}

// =====================================================================================================
// Chi e' stato costruito, e chi no
// =====================================================================================================

namespace
{
	/**
	 * Scende RICORSIVAMENTE. Ogni `UUserWidget` ha un `WidgetTree` suo e `ForEachWidget` non ci entra:
	 * senza questa discesa un `WBP_RT_ActionSlot` dentro il dock resterebbe invisibile, ed e' esattamente
	 * il caso che si voleva vedere.
	 */
	void RTRaccogliWidgetInnestati(const UUserWidget* Radice, TArray<const UUserWidget*>& Fuori)
	{
		if (Radice == nullptr || Radice->WidgetTree == nullptr)
		{
			return;
		}

		Radice->WidgetTree->ForEachWidget([&Fuori](UWidget* Widget)
		{
			if (const UUserWidget* Innestato = Cast<UUserWidget>(Widget))
			{
				Fuori.Add(Innestato);
				RTRaccogliWidgetInnestati(Innestato, Fuori);
			}
		});
	}
}

TArray<FString> URTTacticalHUDWidget::ComposeMountReport(const UUserWidget* Radice)
{
	TArray<FString> Righe;

	if (Radice == nullptr)
	{
		Righe.Add(TEXT("Screen HUD 4.1: nessuna radice, non c'e' albero da camminare"));
		return Righe;
	}

	TArray<const UUserWidget*> Innestati;
	RTRaccogliWidgetInnestati(Radice, Innestati);

	Righe.Add(FString::Printf(TEXT("Screen HUD 4.1 - albero costruito di '%s': %d widget innestati"),
		*Radice->GetName(), Innestati.Num()));

	for (const UUserWidget* Widget : Innestati)
	{
		Righe.Add(FString::Printf(TEXT("  costruito: '%s' (%s)"),
			*Widget->GetName(), *Widget->GetClass()->GetName()));
	}

	// ⚠️ **Gli attesi sono le CLASSI C++, non i nomi degli asset.** Un `.uasset` rinominato continuerebbe a
	// rispondere; un widget sostituito da un segnaposto che ne porta il nome, no. E' la stessa distinzione
	// che `NoNodeWearsTheNameOfAWidgetWithoutBeingOne` fa sull'albero progettato.
	struct FAtteso
	{
		const TCHAR* Nome;
		UClass* Classe;
	};

	const FAtteso Attesi[] =
	{
		{ TEXT("TurnHeader"),        URTTurnHeaderWidget::StaticClass() },
		{ TEXT("TeamRoster"),        URTTeamRosterWidget::StaticClass() },
		{ TEXT("SelectedUnitPanel"), URTSelectedUnitPanelWidget::StaticClass() },
		{ TEXT("ActionDock"),        URTActionDockWidget::StaticClass() },
		{ TEXT("ActionSlot"),        URTActionSlotWidget::StaticClass() },
		{ TEXT("EventLog"),          URTPlayerEventLogWidget::StaticClass() },
	};

	for (const FAtteso& Atteso : Attesi)
	{
		int32 Quanti = 0;
		for (const UUserWidget* Widget : Innestati)
		{
			if (Widget && Widget->IsA(Atteso.Classe))
			{
				++Quanti;
			}
		}

		Righe.Add(FString::Printf(TEXT("  %s %s: %d"),
			Quanti > 0 ? TEXT("[ok]   ") : TEXT("[MANCA]"), Atteso.Nome, Quanti));
	}

	return Righe;
}

void URTTacticalHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// ⚠️ **Fotografia AL MONTAGGIO: non e' l'albero a regime**, e le due cose possono divergere. Per
	// l'albero vivo, a partita avviata: `rt.Debug.ScreenHud`.
	//
	// 🔴 **La distinzione e' costata un equivoco, e va raccontata perche' era nel verso opposto a quello
	// che sembrava.** Durante `U49`, il 2026-09-10, questo dump ha detto `[MANCA] ActionSlot: 0` e la prima
	// spiegazione fu «li crea il grafo del dock DOPO la radice, e' solo il timing». **Falsa**: il `.uasset`
	// di allora non li costruiva affatto, ed e' il difetto che `#2297` ha chiuso facendo costruire al dock
	// i propri slot. La prova che sembrava smentirlo — migliaia di `Icona non risolta` firmate
	// `'ActionSlot'` — veniva da una sessione con un asset diverso.
	//
	// ⚠️ Quindi uno zero su questa riga vuole una **seconda misura**, non una spiegazione: eseguire
	// `rt.Debug.ScreenHud` prima di concludere in un verso o nell'altro.
	UE_LOG(LogRT, Display,
		TEXT("Screen HUD 4.1 - fotografia AL MONTAGGIO; per l'albero a regime usa 'rt.Debug.ScreenHud'"));

	for (const FString& Riga : ComposeMountReport(this))
	{
		UE_LOG(LogRT, Display, TEXT("%s"), *Riga);
	}
}
