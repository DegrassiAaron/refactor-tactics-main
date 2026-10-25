#include "UI/RTHudViewModel.h"

#include "Turn/RTTurnManager.h"
#include "Turn/RTMatchFormatData.h"
#include "Unit/RTUnit.h"
#include "Ability/RTActionData.h"
#include "Ability/RTCatalogLibrary.h" // l'autorita' su quale slot consuma un'azione
#include "Core/RTGameplayTags.h"      // TAG_Status_Reveal: cosa rende un piano visibile all'avversario
#include "UI/RTIconLibrary.h"         // MakeIconId: la chiave dell'icona si deriva dal tag, non si compone
#include "Player/RTPlayerController.h" // HotkeyLabelFor: il tasto si legge dalla tabella, generiche comprese (#2987, D-397)
#include "Turn/RTReactionLibrary.h"   // ControlSeverityRank: la gravita' dei controlli ha gia' un owner (#2274)
#include "Turn/RTIntentPrivacyLibrary.h"
#include "Ability/RTMovementProfileLibrary.h" // ProfileForPlan: l'autorita' sul profilo, non un secondo lettore
#include "Turn/RTPlanValidationLibrary.h"     // MakePlanFor: il piano da cui il profilo si ricava
#include "Turn/RTHexSim.h"                    // FRTHexSimUnit: `ValidatePlan` lo chiede, e dopo D-190 non lo legge
#include "Combat/RTCombatLibrary.h"           // RefusalForKnownTarget: lo stato Warning legge il rifiuto, non lo rifa'
#include "Map/RTHexMapActor.h"                // la mappa del bersaglio pianificato
#include "Map/RTHexMapAsset.h"                // HasObjectiveCell: il punteggio ha senso solo con un obiettivo (#3635)
#include "UI/RTPlayerEventProjector.h" // la porta autorizzata del feed: il filtro non e' del widget
#include "Turn/RTTurnLog.h"            // FRTTurnLogEntry: il feed consuma il log canonico, non il testo
#include "Turn/RTActionFallbackLibrary.h" // ERTActionInvalidReason: il motivo del tooltip (`#3499`)
#include "Turn/RTTurnLogLibrary.h"        // DescribeInvalidReason: il testo del motivo ha gia' un owner
#include "Terrain/RTTerrainLibrary.h"     // EffectiveTargetingRange: la portata APPLICATA del rifiuto
#include "UI/RTHUD.h"                     // RefusalText: il testo del rifiuto ha gia' un owner

FRTMatchHeaderView URTHudViewModel::BuildMatchHeader(const ARTTurnManager* TurnManager)
{
	FRTMatchHeaderView View;
	if (!TurnManager)
	{
		// Vista neutra, non «zero»: un widget costruito prima del manager deve poter mostrare un trattino.
		return View;
	}

	View.Round = TurnManager->GetTurnNumber();
	// Il limite viene dal FORMATO, mai da una costante: e' la voce del DoD di CP 11.1 che esiste per
	// impedire a un widget di stampare `12`. Qui non c'e' proprio un posto dove scriverlo a mano.
	View.RoundLimit = TurnManager->GetMatchRules().RoundLimit;
	View.Phase = TurnManager->GetPhase();
	// La fase RIPRODOTTA, non quella logica ([D-479]): durante il playback le due divergono.
	View.PlaybackPhase = TurnManager->GetPlaybackPhase();
	View.bResolving = TurnManager->IsResolving();

	// Il progresso sull'obiettivo (`CP 10.2`, `#75`). Si legge dal manager, che e' l'unico a saperlo: il
	// punto lo assegna il Cleanup e nessun widget ha modo di ricalcolarlo — ne' deve, per il §4.1 di
	// `progettazione-hud.md`.
	View.Team0Score = TurnManager->GetTeamScore(0);
	View.Team1Score = TurnManager->GetTeamScore(1);
	// Dal formato, come `RoundLimit` poco sopra: `0` = via per obiettivo disattivata, non «zero punti per
	// vincere».
	View.ScoreToWin = TurnManager->GetMatchRules().ScoreToWin;

	// Il timer risponde solo dentro il Planning, e **solo se un timer esiste davvero**. Fuori resta negativo
	// — «non si applica» — invece di 0, che un widget leggerebbe come «scaduto adesso».
	//
	// ⚠️ La seconda guardia non e' difensiva, chiude un caso REALE. `GetPlanningTimeRemaining()` restituisce
	// `0.f` in due situazioni diverse — timer scaduto e timer **mai impostato** — perche' clampa a zero il
	// risultato di `GetTimerRemaining` su un handle che puo' non esistere; e il timer si imposta solo
	// `if (PlanningSeconds > 0.f)`. Un Planning senza orologio non e' un caso di laboratorio:
	// `RTScenarioSession` chiama `SetPlanningSeconds(0.f)` per le run headless. Senza questa riga un
	// `WBP_RT_TurnHeader` mostrerebbe `0:00` lampeggiante per tutta una partita che di proposito non ha
	// tempo — cioe' il contratto dichiarato in `FRTMatchHeaderView` sarebbe falso proprio dove serve.
	if (View.Phase == ERTMatchPhase::Planning && !View.bResolving && TurnManager->GetPlanningSeconds() > 0.f)
	{
		View.PlanningSecondsRemaining = TurnManager->GetPlanningTimeRemaining();
	}

	// Il countdown del Ready (`#2193`, `#2358`). **Una guardia sola**, e non e' una svista rispetto alle tre
	// qui sopra: `IsReadyCountdownActive()` risponde sulla presenza del timer, non sul suo residuo, e il
	// countdown si arma solo su input esplicito — quindi il varco che affligge `PlanningSecondsRemaining`
	// (fase gia' Planning, timer non ancora armato) qui non esiste. Aggiungere `Phase == Planning` sarebbe
	// una condizione che non puo' essere falsa: `RequestLockIn()` non arma niente fuori dal Planning.
	if (TurnManager->IsReadyCountdownActive())
	{
		View.ReadyCountdownSecondsRemaining = TurnManager->GetReadyCountdownRemaining();
	}

	// La finestra di preparazione dell'autobattle (`#2386`). Stessa forma del countdown e per la stessa
	// ragione: `IsPrepWindowActive()` risponde sulla presenza, non sul residuo — e in pausa risponde vero
	// con il timer spento, che e' precisamente il caso in cui il numero deve restare a schermo.
	if (TurnManager->IsPrepWindowActive())
	{
		View.PrepWindowSecondsRemaining = TurnManager->GetPrepWindowRemaining();
		View.bPrepWindowPaused = TurnManager->IsPrepWindowPaused();
	}

	// Il derivato si pubblica **dopo** i due orologi, ed e' l'unico ordine possibile: legge
	// entrambi. Chi riceve la vista intera trova il numero gia' fatto; chi la costruisce a mano
	// chiama la funzione.
	View.SecondsUntilCommit = ComputeSecondsUntilCommit(View);

	return View;
}

FString URTHudViewModel::ComposePhaseWord(const FRTMatchHeaderView& Header)
{
	// Il countdown del Ready e la finestra di preparazione hanno la propria parola: la fase e' ancora Planning,
	// e i due stati devono distinguersi senza contare i secondi (`#2358`, `#2386`).
	const bool bReadyCountdown = Header.ReadyCountdownSecondsRemaining >= 0.f;
	const bool bPrepWindow = Header.PrepWindowSecondsRemaining >= 0.f;
	switch (Header.Phase)
	{
	case ERTMatchPhase::Planning:
		return bPrepWindow ? (Header.bPrepWindowPaused ? TEXT("Preparazione (in pausa)") : TEXT("Preparazione"))
			: bReadyCountdown ? TEXT("Ready")
			: TEXT("Pianificazione");
	case ERTMatchPhase::MatchEnded:
		return TEXT("Fine");
	default:
		return TEXT("Risoluzione");
	}
}

bool URTHudViewModel::MapDeclaresObjective(const ARTTurnManager* TurnManager)
{
	if (!TurnManager)
	{
		return false;
	}
	// La stessa mappa che il Cleanup valuta: quella dell'attore nel mondo, via `GetHexContext` dell'attore (quello del
	// manager e' protetto, e ci inoltra).
	const ARTHexMapActor* Attore = ARTHexMapActor::FindInWorld(TurnManager->GetWorld());
	if (!Attore)
	{
		return false;
	}
	FVector Origine;
	float Lato = 0.f;
	float Piano = 0.f;
	const URTHexMapAsset* Mappa = Attore->GetHexContext(Origine, Lato, Piano);
	return Mappa && Mappa->HasObjectiveCell();
}

float URTHudViewModel::ReadPlaybackProgress01(const ARTTurnManager* TurnManager)
{
	return TurnManager ? TurnManager->GetPlaybackProgress01() : 0.f;
}

float URTHudViewModel::ReadViewerPlaybackSpeed(const ARTTurnManager* TurnManager)
{
	return TurnManager ? TurnManager->ViewerPlaybackSpeed : 0.f;
}

FRTLiveGestures URTHudViewModel::ReadLiveGestures(const ARTTurnManager* TurnManager, int32 PlayerTeamId,
	const TArray<ARTUnit*>& Units)
{
	// Senza manager nessun tasto si nomina: e' il verso sicuro di un suggerimento, che non deve promettere cio' che
	// non sa. Il default della struct e' gia' tutto spento.
	FRTLiveGestures Gestures;
	if (!TurnManager)
	{
		return Gestures;
	}

	// `Spazio` e `RMB` escono entrambi da `IsPlanningInputInert()` nel controller, che legge il GameMode; il
	// `TurnManager` e' informato dal bootstrapper nel punto in cui quel valore e' appena stato latchato, quindi i due
	// lettori dicono la stessa cosa.
	const bool bUnattended = TurnManager->IsUnattendedSession();
	Gestures.bSkip = !bUnattended;
	Gestures.bUnready = !bUnattended;
	Gestures.bSpeed = !ARTPlayerController::PlayerCommandsAnyLiveUnit(PlayerTeamId, Units);
	return Gestures;
}

float URTHudViewModel::ComputeSecondsUntilCommit(const FRTMatchHeaderView& Header)
{
	// Nessun commit in arrivo fuori dal Planning, ne' mentre la risoluzione scorre (revisione di #3849). La riga di
	// stato ritirata (D-539) lo garantiva con un `Phase == Planning` suo; ora lo garantisce la sede del numero, cosi' un
	// secondo chiamante che costruisca la vista a mano - il viewer di replay - non mostra un conto alla rovescia su una
	// partita finita. Durante la riproduzione `Phase` resta `Planning`: la sola `bResolving` lo distingue.
	if (Header.Phase != ERTMatchPhase::Planning || Header.bResolving)
	{
		return -1.f;
	}

	// La finestra dell'autobattle (`#2386`) ha la precedenza su tutto, e non e' una priorita' arbitraria: e'
	// l'ULTIMO orologio della catena. Quando e' armata il Planning e' gia' scaduto — la arma proprio la sua
	// scadenza — quindi non c'e' nessun tetto che possa accorciarla, e in autobattle nessuno preme Ready.
	// ⚠️ In pausa il numero e' fermo, e resta quello giusto da mostrare: il commit arrivera' fra tanti
	// secondi quanti ne restano, appena si riprende.
	if (Header.PrepWindowSecondsRemaining >= 0.f)
	{
		return Header.PrepWindowSecondsRemaining;
	}

	// Il countdown del Ready, quando e' armato, non decide da solo: il tetto del Planning lo accorcia se
	// scade prima (`#2193`). `>= 0.f` e non `> 0.f` — uno zero qui significa «commit adesso», che e' un
	// numero da mostrare, non un caso da escludere.
	if (Header.ReadyCountdownSecondsRemaining >= 0.f)
	{
		// ⚠️ Il tetto entra **solo se si applica**: nelle run headless `PlanningSecondsRemaining` e'
		// negativo, e un `Min` cieco restituirebbe quel negativo spegnendo l'unico orologio in corsa.
		return (Header.PlanningSecondsRemaining > 0.f)
			? FMath::Min(Header.ReadyCountdownSecondsRemaining, Header.PlanningSecondsRemaining)
			: Header.ReadyCountdownSecondsRemaining;
	}

	// Senza Ready il commit arriva alla scadenza del Planning — e se quel campo dice «non si applica», la
	// risposta e' la stessa: non c'e' nessun conto alla rovescia da mostrare.
	return Header.PlanningSecondsRemaining;
}

FRTUnitCardView URTHudViewModel::BuildUnitCard(const ARTUnit* Unit, int32 PlayerTeamId)
{
	FRTUnitCardView Card;
	if (!Unit)
	{
		return Card;
	}

	Card.HeroId = Unit->HeroId;
	Card.DisplayName = FText::FromString(ARTUnit::DisplayLabel(Unit->HeroDisplayName, Unit->HeroId, Unit->GetName()));
	Card.Health = Unit->Health;
	Card.MaxHealth = Unit->MaxHealth;
	Card.Shield = Unit->Shield;
	Card.bIsAlly = (Unit->TeamId == PlayerTeamId);
	Card.bAlive = Unit->IsAlive();

	// `MaxHealth <= 0` non e' un errore da segnalare: e' la card VUOTA — quella che il pannello mostra
	// senza selezione — e la barra giusta e' quella a zero.
	//
	// `Clamp` e non aritmetica nuda: il contratto dichiara `[0,1]` e `Health` viene dal simulatore, dove
	// una cura o un danno eccessivo possono portarlo fuori dall'intervallo. E' la stessa cura gia' presa
	// da `ChargeFraction` poco piu' sotto.
	Card.HealthFraction = (Card.MaxHealth > 0)
		? FMath::Clamp(static_cast<float>(Card.Health) / static_cast<float>(Card.MaxHealth), 0.f, 1.f)
		: 0.f;

	return Card;
}

namespace
{
	/** Riempie uno slot con l'azione che lo occupa, prendendone identita' e nome dal kit. */
	void FillSlotFromAbility(FRTPlannedSlotView& Slot, const ARTUnit& Unit, int32 AbilityIndex)
	{
		const URTActionData* Action = Unit.GetAbility(AbilityIndex);
		if (!Action)
		{
			return; // indice non valido: lo slot resta libero invece di risultare occupato da nulla
		}
		Slot.bOccupied = true;
		Slot.ActionId = Action->Def.ActionId;
		Slot.DisplayName = Action->DisplayName;
	}
}

FRTUnitSlotsView URTHudViewModel::BuildUnitSlots(const ARTUnit* Unit)
{
	FRTUnitSlotsView Slots;
	if (!Unit)
	{
		return Slots;
	}

	// ⚠️ **`bAuthorized` lo alza il fatto stesso di costruire, e la difesa non e' qui.** Questa funzione non
	// sa CHI comanda l'unita' che le viene passata: chiederglielo significherebbe darle una seconda regola di
	// autorizzazione accanto a quella di chi chiama. La barriera sta nel chiamante —
	// `URTSelectedUnitPanelWidget::GetSlots()` non invoca questa funzione per un soggetto ispezionato, quindi
	// il piano di un'avversaria non viene **costruito**, non «costruito e poi nascosto».
	Slots.bAuthorized = true;

	// --- movimento ---------------------------------------------------------------------------------------
	// Due modi di occuparlo, e uno solo dei due ha un nome. Un percorso e' `PlannedWaypoints`: occupa lo slot
	// e non e' un'azione scelta, quindi resta senza `ActionId` (vedi il commento su `FRTPlannedSlotView`).
	if (Unit->PlannedWaypoints.Num() > 0)
	{
		Slots.Movement.bOccupied = true;
	}
	if (Unit->PlannedDashAbility != INDEX_NONE)
	{
		// Lo scatto NOMINA lo slot anche se un percorso lo aveva gia' segnato occupato: fra i due e' l'unico
		// che il giocatore ha scelto per nome, ed e' l'informazione utile. Che i due possano coesistere non lo
		// decide la HUD — se e' un piano illegale, lo dice `ValidateActionSlots`.
		FillSlotFromAbility(Slots.Movement, *Unit, Unit->PlannedDashAbility);
	}

	// --- azione principale -------------------------------------------------------------------------------
	if (Unit->PlannedAbilityIndex != INDEX_NONE)
	{
		FillSlotFromAbility(Slots.Main, *Unit, Unit->PlannedAbilityIndex);

		// ⚠️ Un'azione che dichiara `MovementAndMain` e' un'azione sola e DUE slot. Senza questa riga il
		// pannello mostrerebbe il movimento libero a chi ha speso tutto il turno in una mobilita' che costa
		// entrambi. Oggi nessuna azione lo dichiara - `Action.Sprint` lo faceva fino a [D-028] - quindi la
		// riga e' inerte, non superflua: torna a contare il giorno che un kit usa quella forma.
		// Chi risponde e' il catalogo, non un `==` scritto qui: e' lo stesso predicato del validatore.
		if (const URTActionData* Main = Unit->GetAbility(Unit->PlannedAbilityIndex))
		{
			if (URTCatalogLibrary::TakesMovementSlot(Main->Def))
			{
				FillSlotFromAbility(Slots.Movement, *Unit, Unit->PlannedAbilityIndex);
			}
		}
	}

	// --- il profilo dello slot movimento -----------------------------------------------------------------
	// 🔑 **Si interroga `ProfileForPlan`, non si legge `PlannedMovementProfileId`.** E' la stessa autorita'
	// che `ARTTurnManager::MakeSimUnit` usa per ricavare i due budget: qualunque altra lettura sarebbe una
	// seconda derivazione, e la HUD mostrerebbe un profilo con cui il turno non si risolve.
	//
	// → Chi non ha mai toccato il selettore legge `Move` con un percorso e `Still` senza: e' la stessa
	// risposta che dara' il resolver, non un caso speciale della vista.
	Slots.MovementProfileId = URTMovementProfileLibrary::ProfileForPlan(
		URTPlanValidationLibrary::MakePlanFor(Unit)).Id;

	// --- reazione ----------------------------------------------------------------------------------------
	// Slot indipendente (CP 5.1): un eroe puo' muoversi, agire E tenere una reazione pronta nello stesso turno.
	if (Unit->PlannedReactionAbility != INDEX_NONE)
	{
		FillSlotFromAbility(Slots.Reaction, *Unit, Unit->PlannedReactionAbility);
	}

	// [D-478] punto 2 (#3618): la reazione armata e' lo slot `Reaction` occupato da un'azione, oppure una principale
	// che arma una finestra. La domanda la decide il catalogo, come `TakesMovementSlot` decide gli slot.
	const URTActionData* Reazione = Unit->GetAbility(Unit->PlannedReactionAbility);
	const URTActionData* Principale = Unit->GetAbility(Unit->PlannedAbilityIndex);
	Slots.bReactionArmed = (Reazione && URTCatalogLibrary::ArmsReaction(Reazione->Def))
		|| (Principale && URTCatalogLibrary::ArmsReaction(Principale->Def));

	return Slots;
}

FText URTHudViewModel::PlanActionDisplayName(const ARTUnit* Unit, const TArray<FRTPlannedAction>& Plan,
	const FName& ActionId)
{
	// 1. Il nome del kit: la prima voce con quell'id e un nome leggibile. La ricerca e' di `ARTUnit`, che la
	//    tiene in un posto solo — la usa anche il titolo della finestra di reazione (`#3667`). Un nome di soli
	//    spazi non conta: a schermo e' una riga vuota, e la cascata deve poter proseguire.
	if (Unit)
	{
		const FText DalKit = Unit->KitDisplayNameFor(ActionId);
		if (!DalKit.IsEmpty())
		{
			return DalKit;
		}
	}
	// 2. La voce di movimento, che `MakePlanFor` aggiunge dal core.
	for (const FRTPlannedAction& Voce : Plan)
	{
		if (Voce.Def.ActionId == ActionId && URTCatalogLibrary::TakesMovementSlot(Voce.Def))
		{
			return NSLOCTEXT("RTHud", "TooltipSlotMovement", "Movimento");
		}
	}
	// 3. La rete: l'ultimo segmento dell'id.
	FString Id = ActionId.ToString();
	int32 Punto = INDEX_NONE;
	return FText::FromString(Id.FindLastChar(TEXT('.'), Punto) ? Id.Mid(Punto + 1) : Id);
}

TArray<FRTPlanWarningView> URTHudViewModel::BuildPlanWarnings(const ARTUnit* Unit, const TArray<ARTUnit*>& Units)
{
	TArray<FRTPlanWarningView> Avvisi;
	if (!Unit || !Unit->IsAlive())
	{
		return Avvisi;
	}

	const TArray<FRTPlannedAction> Piano = URTPlanValidationLibrary::MakePlanFor(Unit);

	// Il nome di un'azione del piano: la cascata e' `PlanActionDisplayName`, qui sopra.
	const auto NomeDi = [Unit, &Piano](const FName& ActionId) -> FText
	{
		return PlanActionDisplayName(Unit, Piano, ActionId);
	};

	// --- 1. Critical: il validatore. Lo dice, non lo decide. -------------------------------------------------
	const FRTPlanValidation Verdetto = URTPlanValidationLibrary::ValidatePlan(FRTHexSimUnit(), Piano);
	if (!Verdetto.bLegal)
	{
		FRTPlanWarningView Critico;
		Critico.Level = ERTPlanWarningLevel::Critical;
		Critico.SourceActionId = Verdetto.OffendingActionId;
		Critico.What = NomeDi(Verdetto.OffendingActionId);
		// Le due azioni di un conflitto di slot, come il log di lock-in (`RTTurnManager.cpp`): nominarne una sola
		// manderebbe a correggere quella sbagliata.
		Critico.Why = Verdetto.HolderActionId.IsNone()
			? FText::FromString(URTTurnLogLibrary::DescribeInvalidReason(Verdetto.Reason))
			: FText::Format(NSLOCTEXT("RTPlanWarning", "SlotConflict", "{0} e {1} occupano lo stesso slot"),
				NomeDi(Verdetto.OffendingActionId), NomeDi(Verdetto.HolderActionId));
		// ⚠️ Il costo e' cio' che si sa, non una previsione: il lock-in non scarta niente, e cio' che il turno
		// scarta lo decide il resolver.
		Critico.Cost = NSLOCTEXT("RTPlanWarning", "IllegalCost", "Il validatore lo giudica illegale: correggilo prima di confermare");
		Avvisi.Add(Critico);
	}

	// Una voce per AZIONE (review di #3622, §15.1): un'azione che il validatore giudica illegale probabilmente non
	// verra' eseguita, e dirne anche il ripiego o il fuoco amico accenderebbe tre voci per un problema solo.
	const auto CopertaDalCritico = [&Verdetto](const FName& ActionId)
	{
		return !Verdetto.bLegal && !ActionId.IsNone()
			&& (ActionId == Verdetto.OffendingActionId || ActionId == Verdetto.HolderActionId);
	};

	// --- 2. Warning: il bersaglio che in risoluzione prende il ripiego ([D-459]) ------------------------------
	for (const FRTAbilityCooldownView& Riga : BuildAbilityCooldowns(Unit))
	{
		if (!Riga.bPlanDegraded || CopertaDalCritico(Riga.ActionId))
		{
			continue;
		}
		FRTPlanWarningView Degradato;
		Degradato.Level = ERTPlanWarningLevel::Warning;
		Degradato.SourceActionId = Riga.ActionId;
		Degradato.What = Riga.DisplayName.IsEmpty() ? FText::FromName(Riga.ActionId) : Riga.DisplayName;
		Degradato.Why = FText::FromString(ARTHUD::RefusalText(Riga.PlanDegradedRefusal, Riga.PlanDegradedRange));
		Degradato.Cost = NSLOCTEXT("RTPlanWarning", "DegradedCost", "In risoluzione prende il ripiego");
		Avvisi.Add(Degradato);
	}

	// --- 3. Warning: il fuoco amico del PROPRIO piano ([D-492], [D-494]) ------------------------------------
	const int32 Se = Units.IndexOfByKey(const_cast<ARTUnit*>(Unit));
	if (Se != INDEX_NONE)
	{
		const FRTBlastPreview Blast = URTHexCombatLibrary::MakeBlastPreview(
			MakeBlastPreviewPlan(*Unit, Se, Units), MakeHexCombatUnits(Units));
		TArray<FString> Alleate;
		for (int32 i = 0; i < Units.Num(); ++i)
		{
			const ARTUnit* Altra = Units[i];
			if (i != Se && Altra && Altra->IsAlive() && Altra->TeamId == Unit->TeamId && Blast.AllyCells.Contains(Altra->Cell))
			{
				Alleate.Add(ARTUnit::DisplayLabel(Altra->HeroDisplayName, Altra->HeroId, Altra->GetName()));
			}
		}
		const URTActionData* Principale = Unit->GetAbility(Unit->PlannedAbilityIndex);
		if (Alleate.Num() > 0 && !(Principale && CopertaDalCritico(Principale->Def.ActionId)))
		{
			FRTPlanWarningView FuocoAmico;
			FuocoAmico.Level = ERTPlanWarningLevel::Warning;
			FuocoAmico.SourceActionId = Principale ? Principale->Def.ActionId : NAME_None;
			FuocoAmico.What = NomeDi(FuocoAmico.SourceActionId);
			FuocoAmico.Why = FText::Format(NSLOCTEXT("RTPlanWarning", "FriendlyFireWhy", "Nella zona: {0}"),
				FText::FromString(FString::Join(Alleate, TEXT(", "))));
			FuocoAmico.Cost = NSLOCTEXT("RTPlanWarning", "FriendlyFireCost", "Colpisce anche le alleate nella zona");
			Avvisi.Add(FuocoAmico);
		}
	}

	// --- 4. Warning: la carica che NON arriva dove il giocatore ha cliccato ([D-516] `#3736`, [D-528] `#3769`) -----
	//
	// 🔑 **[D-528] Un altro nemico noto sulla retta**: la carica lo urta e manca quello cliccato, in portata come oltre. E' UN
	// avviso solo: «oltre la portata» sarebbe vero ma secondario, perche' la corsa si ferma prima di arrivarci.
	FRTChargeRun Lettura;
	const bool bColpisceUnAltro = Unit->GetPlannedChargeRun(Lettura) && Lettura.bHasImpact && Lettura.bMissesTarget;
	if (bColpisceUnAltro)
	{
		const URTActionData* Carica = Unit->GetAbility(Unit->PlannedDashAbility);
		const FName CaricaId = Carica ? Carica->Def.ActionId : NAME_None;
		if (!CopertaDalCritico(CaricaId))
		{
			// I nomi si cercano per CELLA fra le unita' che il chiamante passa: l'urtato e' una cella che la squadra conosce
			// (la lettura viene dalla sua fotografia), e se li' non c'e' piu' nessuno la frase resta senza nome invece di inventarne uno.
			// 🔴 `IsKnownToObserver()`: `Units` e' TUTTO il mondo, e la cella e' una cella: un nemico che il velo nasconde non si nomina
			// ([D-225]). Il bersaglio, poi, si nomina solo se la lettura dice che e' un nemico noto (`bTargetIsKnownEnemy`).
			const auto EtichettaSu = [&Units, Unit](const FRTCellId& Cella) -> FString
			{
				for (const ARTUnit* Altra : Units)
				{
					if (Altra && Altra->IsAlive() && Altra->TeamId != Unit->TeamId && Altra->Cell == Cella
						&& Altra->IsKnownToObserver())
					{
						return ARTUnit::DisplayLabel(Altra->HeroDisplayName, Altra->HeroId, Altra->GetName());
					}
				}
				return FString();
			};
			const FString Colpito = EtichettaSu(Lettura.ImpactCell);
			const FString Voluto = Lettura.bTargetIsKnownEnemy ? EtichettaSu(Lettura.Target) : FString();

			FRTPlanWarningView Altro;
			Altro.Level = ERTPlanWarningLevel::Warning;
			Altro.SourceActionId = CaricaId;
			Altro.What = NomeDi(CaricaId);
			Altro.Why = Colpito.IsEmpty()
				? NSLOCTEXT("RTPlanWarning", "ChargeOtherEnemyWhyUnnamed", "Sulla retta c'è un altro nemico")
				: FText::Format(NSLOCTEXT("RTPlanWarning", "ChargeOtherEnemyWhy", "Sulla retta c'è un altro nemico: {0}"),
					FText::FromString(Colpito));
			// ⚠️ «Colpirà», non «colpirà solo»: la risoluzione e' simultanea e puo' cambiare cio' che sta sulla linea.
			Altro.Cost = Colpito.IsEmpty()
				? NSLOCTEXT("RTPlanWarning", "ChargeOtherEnemyCostUnnamed", "Colpirà un altro nemico, non quello scelto")
				: (Voluto.IsEmpty()
					? FText::Format(NSLOCTEXT("RTPlanWarning", "ChargeOtherEnemyCostNoTarget", "Colpirà {0}, non arriva alla cella scelta"),
						FText::FromString(Colpito))
					: FText::Format(NSLOCTEXT("RTPlanWarning", "ChargeOtherEnemyCost", "Colpirà {0}, non {1}"),
						FText::FromString(Colpito), FText::FromString(Voluto)));
			Avvisi.Add(Altro);
		}
	}

	// [D-516] Il bersaglio OLTRE la portata. 🔑 **Riporta, non decide**: il numero viene da `PlannedChargeReachBeyondRange`, la
	// stessa portata che la pianificazione ha accettato e che il resolver eseguira'. Il piano e' legale — la carica corre fino
	// alla portata e si ferma —, ed e' degradato: non colpisce il bersaglio scelto. E' la definizione di `Warning`.
	int32 Distanza = 0;
	if (const int32 Corsa = bColpisceUnAltro ? 0 : Unit->PlannedChargeReachBeyondRange(&Distanza); Corsa > 0)
	{
		const URTActionData* Carica = Unit->GetAbility(Unit->PlannedDashAbility);
		const FName CaricaId = Carica ? Carica->Def.ActionId : NAME_None;
		if (!CopertaDalCritico(CaricaId))
		{
			FRTPlanWarningView Corta;
			Corta.Level = ERTPlanWarningLevel::Warning;
			Corta.SourceActionId = CaricaId;
			Corta.What = NomeDi(CaricaId);
			Corta.Why = FText::Format(NSLOCTEXT("RTPlanWarning", "ChargeBeyondRangeWhy",
				"Il bersaglio è oltre la portata ({0} celle, portata {1})"), FText::AsNumber(Distanza), FText::AsNumber(Corsa));
			// ⚠️ **«Non lo raggiunge», non «senza colpire»** (review di #3736): la corsa colpisce ancora chi sta sulla
			// linea entro la portata all'inizio del Dash, compreso un nemico che chi pianifica non vede. Promettere che
			// non colpira' nessuno sarebbe dire al giocatore una cosa che il resolver non mantiene. E «al piu'», perche' quel
			// nemico la ferma prima della portata (seconda revisione della PR #3767).
			Corta.Cost = FText::Format(NSLOCTEXT("RTPlanWarning", "ChargeBeyondRangeCost",
				"Non lo raggiunge: corre al più per {0} celle"), FText::AsNumber(Corsa));
			Avvisi.Add(Corta);
		}
	}
	return Avvisi;
}

void URTHudViewModel::AddPlanWarningCounts(const TArray<FRTPlanWarningView>& Warnings, FRTPlanWarningCounts& InOut)
{
	for (const FRTPlanWarningView& Avviso : Warnings)
	{
		switch (Avviso.Level)
		{
		case ERTPlanWarningLevel::Critical: ++InOut.Critical; break;
		case ERTPlanWarningLevel::Warning:  ++InOut.Warning;  break;
		case ERTPlanWarningLevel::Info:     ++InOut.Info;     break;
		}
	}
}

FRTBlastPreviewPlan URTHudViewModel::MakeBlastPreviewPlan(const ARTUnit& Unit, int32 UnitId,
	const TArray<ARTUnit*>& Units)
{
	FRTBlastPreviewPlan Plan;
	Plan.AttackerId = UnitId;
	Plan.bDashResolves = Unit.PlannedDashMoves(); // [D-471]: uno scatto negato dallo stato non sposta
	Plan.PlannedDashCell = Unit.PlannedDashCell;
	Plan.bDashIsCharge = Unit.PlannedDashIsCharge();

	const URTActionData* Ability = Unit.GetAbility(Unit.PlannedAbilityIndex);
	if (!Ability)
	{
		return Plan; // solo scatto: un'origine da mostrare e nessuna area
	}

	Plan.bHasAction = true;
	// [D-464]: l'area parte da dove l'azione MIRA, e quello dipende dalla sua fase.
	Plan.Phase = Ability->Def.ResolutionPhase;
	Plan.Shape = Ability->Shape;
	Plan.RangeCells = Ability->RangeCells;
	Plan.AreaRadius = Ability->AreaRadius;
	// Il fuoco amico solo se l'azione puo' DAVVERO colpire i propri: un allarme su un evento impossibile insegna
	// a ignorare gli allarmi.
	Plan.bFriendlyFire = Ability->Def.bFriendlyFire;
	if (Unit.bAttackTargetsCell)
	{
		Plan.bTargetsCell = true;
		Plan.TargetCell = Unit.PlannedAttackCell;
	}
	else
	{
		// Un bersaglio che non e' in `Units` diventa `INDEX_NONE`, cioe' «nessuna area»; uno caduto ha `bAlive`
		// falso in `MakeHexCombatUnits`, e l'anteprima lo scarta allo stesso modo.
		Plan.TargetId = Units.IndexOfByKey(Unit.PlannedAttackTarget.Get());
	}
	return Plan;
}

TArray<FRTHexCombatUnit> URTHudViewModel::MakeHexCombatUnits(const TArray<ARTUnit*>& Units)
{
	TArray<FRTHexCombatUnit> HexUnits;
	HexUnits.Reserve(Units.Num());
	for (int32 i = 0; i < Units.Num(); ++i)
	{
		FRTHexCombatUnit HU;
		HU.UnitId = i;
		HU.TeamId = Units[i] ? Units[i]->TeamId : INDEX_NONE;
		HU.Cell = Units[i] ? Units[i]->Cell : FRTCellId();
		HU.bAlive = Units[i] && Units[i]->IsAlive();
		HexUnits.Add(HU);
	}
	return HexUnits;
}

FRTUnitOverlayView URTHudViewModel::BuildUnitOverlay(const ARTUnit* Unit, int32 PlayerTeamId,
	const TSet<FRTCellId>& PlannedHitCells, const TSet<FRTCellId>& PlannedAllyHitCells)
{
	FRTUnitOverlayView View;
	if (Unit == nullptr)
	{
		// Vista neutra e non «zero»: un widget costruito prima dell'unita' mostra un vuoto, non un morto a
		// 0 HP. E' la stessa scelta di `BuildMatchHeader`.
		return View;
	}

	View.Card = BuildUnitCard(Unit, PlayerTeamId);
	View.Statuses = BuildStatusBadges(Unit);

	// Il nome CANONICO del catalogo ([D-120]), non l'ID stabile: `Hero.Aevik` si legge `Aevik`. Il
	// ripiego sull'ID resta dentro `DisplayLabel`, per le unita' che nessun eroe ha configurato.
	View.DisplayName = ARTUnit::DisplayLabel(Unit->HeroDisplayName, Unit->HeroId, Unit->GetName());

	// Gli stessi due colori che il canvas usava, e presi dalla stessa porta: chi guarda vede i propri in
	// azzurro e gli altri in rosso, **non** «team 0 azzurro» — la distinzione conta per lo spettatore.
	View.TeamColor = ARTUnit::TeamColorFor(View.Card.bIsAlly ? 0 : 1,
		FLinearColor(0.55f, 0.75f, 1.f, 1.f), FLinearColor(1.f, 0.62f, 0.55f, 1.f));

	// 🔴 **Chi viene colpito, marcato sull'UNITA' e non solo sulla cella.** L'anteprima a terra dice quali
	// CELLE entrano nella zona; la domanda che ci si fa guardando lo schermo e' un'altra — «questo cilindro
	// lo prendo o no?». Finche' c'era solo la prima, l'anteprima si vedeva e non si capiva (osservato in PIE
	// il 2026-08-08).
	//
	// ⚠️ **Il fuoco amico VINCE sul bersaglio**: e' l'unico caso in cui chi guarda potrebbe voler cambiare
	// idea, e due avvisi sullo stesso nome si annullerebbero a vicenda.
	View.bFriendlyFire = PlannedAllyHitCells.Contains(Unit->Cell);
	View.bTargeted = !View.bFriendlyFire && PlannedHitCells.Contains(Unit->Cell);

	return View;
}

TArray<FRTStatusBadgeView> URTHudViewModel::BuildStatusBadges(const ARTUnit* Unit)
{
	TArray<FRTStatusBadgeView> Badges;
	if (Unit == nullptr)
	{
		return Badges;
	}

	// L'elenco arriva gia' ordinato e senza duplicati, e le due sorgenti (a termine e legata alla cella)
	// sono gia' unite: qui non si enumera niente per conto proprio.
	TArray<FGameplayTag> Tags = Unit->GetActiveStatusTags();
	Badges.Reserve(Tags.Num());

	// L'ordine di presentazione si decide QUI, sui tag, prima di costruire le viste.
	//
	// 🔑 **La gravita' si CHIEDE, non si ricopia.** `ARTHUD::DrawHUD` mostrava `ROOT` e poi `SLOW` in un
	// `if`/`else if`: lo stesso ordine di `ControlStatusesBySeverity`, scritto una seconda volta. Se quella
	// lista cambiasse — o nascesse un terzo controllo — l'HUD sarebbe rimasto fermo, e il guardiano
	// `Reaction.ControlStatusesAreThree` non se ne sarebbe accorto: sorveglia la lista, non chi la copia.
	//
	// ✅ **Ed e' successo davvero**: [D-416] ha fatto nascere il terzo controllo (`Status.Stunned`) e questa
	// funzione non e' stata toccata — chiede il rango invece di conoscerlo. E' la prova che la duplicazione
	// era chiusa, non solo dichiarata tale.
	//
	// 🔴 **Si ordina sui TAG e non sulle viste** perche' il rango si chiede a un `FGameplayTag`: ordinare
	// dopo costringerebbe a risolvere ogni `FName` indietro con `RequestGameplayTag` — dentro un
	// comparatore, e con una chiamata che puo' fallire — per riottenere cio' che qui si ha gia' in mano.
	//
	// ⚠️ **`StableSort` e non `Sort`**: i non-controlli hanno tutti lo stesso rango, e senza stabilita' il
	// loro ordine relativo — che `GetActiveStatusTags` ha reso deterministico apposta — tornerebbe a
	// dipendere dall'implementazione del sort (invariante #4).
	Tags.StableSort([](const FGameplayTag& A, const FGameplayTag& B)
	{
		const int32 RankA = URTReactionLibrary::ControlSeverityRank(A);
		const int32 RankB = URTReactionLibrary::ControlSeverityRank(B);
		const bool bControlA = (RankA != INDEX_NONE);
		const bool bControlB = (RankB != INDEX_NONE);

		if (bControlA != bControlB)
		{
			return bControlA; // i controlli davanti
		}
		if (!bControlA)
		{
			return false; // fra non-controlli decide l'ordine gia' stabilito: `StableSort` lo preserva
		}
		return RankA < RankB; // fra due controlli, `0` e' il piu' grave
	});

	for (const FGameplayTag& Tag : Tags)
	{
		FRTStatusBadgeView Badge;
		Badge.Tag = Tag.GetTagName();
		// La chiave dell'icona si DERIVA dall'identificatore stabile, non si compone accanto a lui.
		Badge.IconId = URTIconLibrary::MakeIconId(Tag.GetTagName());

		const int32 Remaining = Unit->GetStatusRemainingTurns(Tag);
		Badge.bCellBound = (Remaining == ARTUnit::PersistentWhileOnCell);
		// 🔴 `PersistentWhileOnCell` non attraversa questo confine: e' `-1`, e sopra la testa di un'unita' si
		// leggerebbe «meno un turno». Chi disegna guarda `bCellBound`, che dice la stessa cosa senza fingere
		// di essere un conteggio.
		Badge.RemainingTurns = Badge.bCellBound ? 0 : FMath::Max(0, Remaining);

		// L'appartenenza alla famiglia dei controlli la dichiara chi la possiede, non un elenco locale.
		Badge.bIsControl = (URTReactionLibrary::ControlSeverityRank(Tag) != INDEX_NONE);

		Badges.Add(Badge);
	}

	return Badges;
}

ERTActionSlotState URTHudViewModel::ResolveSlotState(const FRTAbilityCooldownView& Action, bool bArmed)
{
	// L'ordine di questi `if` E' la precedenza, ed e' l'unica sede in cui esiste.

	// Nessuna azione: nient'altro puo' valere, e il campo che lo dice e' `ActionId` (`#2987`).
	if (Action.ActionId.IsNone())
	{
		return ERTActionSlotState::Empty;
	}

	// [D-459] **«Non lo potrai fare» batte anche «cosa sto per fare»**: lo slot armato col bersaglio puntato
	// rifiutato (lettura A), o lo slot colpevole di un piano illegale (lettura B). E' la sola eccezione alla
	// regola di `ComposeAbilityLine` qui sotto, e la ragione e' che il bianco dell'armata nasconderebbe il
	// motivo per cui il click non partira'.
	if ((bArmed && Action.bTargetRefused) || Action.bPlanInvalid)
	{
		return ERTActionSlotState::Invalid;
	}

	// 🔑 **Armata batte tutto il resto, ed e' la regola di `ARTHUD::ComposeAbilityLine`**, non una nuova:
	// *«"Cosa sto per fare" e "posso farlo" sono due domande, e il bianco risponde alla prima»*.
	//
	// ⏱️ *Qui c'era «un'ultimate armata e ancora in ricarica resta riconoscibile come quella scelta», come se fosse la coppia
	// che l'input forma armando.* Da [D-531] (`#3830`) l'input non la forma piu': un'azione in ricarica non si arma. La forma
	// la RISOLUZIONE: per un'unita' viva, un'azione consumata resta armata, e in ricarica, fino al Cleanup, e la vista che il
	// dock costruisce la risolve cosi' mentre una finestra di reazione e' aperta nel Move (misurato: `Selected` con la ricarica
	// a vista, `Reactions.AConsumedArmedActionReachesTheDockAsSelectedWhileAWindowIsOpen`; che il widget la mostri e' lettura del codice). La precedenza risponde `Selected`, ma fu pensata per la coppia da input: che cosa
	// mostrare per un'azione appena spesa non l'ha deciso nessuno, e qui resta com'e'.
	if (bArmed)
	{
		return ERTActionSlotState::Selected;
	}

	// [D-459] Il piano accettato ma DEGRADATO batte il semplice «pianificato»: e' ancora un impegno preso, e
	// in piu' dice che in risoluzione prendera' il ripiego.
	if (Action.bPlanDegraded)
	{
		return ERTActionSlotState::Warning;
	}

	// Gia' nel piano: un impegno preso, che sopravvive al fatto che l'armamento sia passato ad altro.
	if (Action.bPlanned)
	{
		return ERTActionSlotState::Planned;
	}

	// La ricarica prima dell'indisponibilita' generica: e' l'unico motivo che il giocatore puo' leggere come
	// un numero, e passa da solo col tempo.
	if (Action.TurnsRemaining > 0)
	{
		return ERTActionSlotState::Cooldown;
	}

	// ⚠️ Da [D-324] `bUsableNow` coincide con `TurnsRemaining == 0`, quindi questo ramo oggi non si
	// raggiunge da una vista costruita dal simulatore. Resta perche' la vista e' una struct e un chiamante
	// puo' comporla cosi' — ed e' la stessa ragione per cui `AbilityLineShowsCooldownOnly` prova quel caso.
	if (!Action.bUsableNow)
	{
		return ERTActionSlotState::Unavailable;
	}

	return ERTActionSlotState::Available;
}

namespace
{
	/** «1 turno» e «2 turni»: il numero e la sua parola, accordati. */
	FText TooltipCount(int32 N, const FText& One, const FText& Many)
	{
		return FText::Format(NSLOCTEXT("RTHud", "TooltipCount", "{0} {1}"), FText::AsNumber(N), N == 1 ? One : Many);
	}

	FRTActionTooltipLine TooltipLine(const FText& Label, const FText& Value)
	{
		FRTActionTooltipLine Line;
		Line.Label = Label;
		Line.Value = Value;
		return Line;
	}
}

FRTActionTooltipView URTHudViewModel::BuildCatalogTooltip(const FRTAbilityCooldownView& Action)
{
	FRTActionTooltipView Out;
	if (Action.ActionId.IsNone())
	{
		return Out; // una posizione di kit vuota non ha niente da spiegare
	}
	Out.Title = Action.DisplayName.IsEmpty() ? FText::FromName(Action.ActionId) : Action.DisplayName;
	Out.Description = Action.Description;

	// ── Le righe: fase, slot, portata, ricarica, danno ──────────────────────────────────────────────────
	if (Action.PhaseMark != ERTActionPhaseMark::None && !Action.PhaseLabel.IsEmpty())
	{
		Out.Lines.Add(TooltipLine(NSLOCTEXT("RTHud", "TooltipPhase", "Fase"), Action.PhaseLabel));
	}
	switch (Action.Slot)
	{
	case ERTActionSlot::Main:
		Out.Lines.Add(TooltipLine(NSLOCTEXT("RTHud", "TooltipSlot", "Slot"), NSLOCTEXT("RTHud", "TooltipSlotMain", "Principale")));
		break;
	case ERTActionSlot::Movement:
		Out.Lines.Add(TooltipLine(NSLOCTEXT("RTHud", "TooltipSlot", "Slot"), NSLOCTEXT("RTHud", "TooltipSlotMovement", "Movimento")));
		break;
	case ERTActionSlot::Reaction:
		Out.Lines.Add(TooltipLine(NSLOCTEXT("RTHud", "TooltipSlot", "Slot"), NSLOCTEXT("RTHud", "TooltipSlotReaction", "Reazione")));
		break;
	case ERTActionSlot::None:
		break; // un'azione che non occupa slot non ne dichiara uno
	}
	// ⛔ **Una riga senza valore non c'e'** (#3419, decisione d'autore del 2026-10-06): un'azione su di se' non ha
	// una portata da dire, e una ricarica a zero non ha turni. ⏱️ *Fino ad allora dicevano «Su di te» e «Nessuna».*
	if (!Action.bSelfTarget && Action.RangeCells > 0)
	{
		Out.Lines.Add(TooltipLine(NSLOCTEXT("RTHud", "TooltipRange", "Portata"), TooltipCount(Action.RangeCells,
			NSLOCTEXT("RTHud", "TooltipCell", "cella"), NSLOCTEXT("RTHud", "TooltipCells", "celle"))));
	}
	switch (Action.Shape)
	{
	case ERTAbilityShape::Area:
		Out.Lines.Add(TooltipLine(NSLOCTEXT("RTHud", "TooltipShape", "Forma"), Action.AreaRadius > 0
			? FText::Format(NSLOCTEXT("RTHud", "TooltipShapeAreaRadius", "Area, raggio {0}"), FText::AsNumber(Action.AreaRadius))
			: NSLOCTEXT("RTHud", "TooltipShapeArea", "Area")));
		break;
	case ERTAbilityShape::Line:
		Out.Lines.Add(TooltipLine(NSLOCTEXT("RTHud", "TooltipShape", "Forma"), NSLOCTEXT("RTHud", "TooltipShapeLine", "Linea")));
		break;
	case ERTAbilityShape::Cone:
		Out.Lines.Add(TooltipLine(NSLOCTEXT("RTHud", "TooltipShape", "Forma"), NSLOCTEXT("RTHud", "TooltipShapeCone", "Cono")));
		break;
	case ERTAbilityShape::Single:
		break; // un colpo puntuale non ha una forma da dire
	}
	if (Action.CooldownTurns > 0)
	{
		Out.Lines.Add(TooltipLine(NSLOCTEXT("RTHud", "TooltipCooldown", "Ricarica"), TooltipCount(Action.CooldownTurns,
			NSLOCTEXT("RTHud", "TooltipTurn", "turno"), NSLOCTEXT("RTHud", "TooltipTurns", "turni"))));
	}
	if (Action.Damage > 0)
	{
		Out.Lines.Add(TooltipLine(NSLOCTEXT("RTHud", "TooltipDamage", "Danno"), FText::AsNumber(Action.Damage)));
	}

	// Il compromesso della variante attiva, accanto ai numeri e non al loro posto (#3419). Senza variante non c'e'.
	if (!Action.VariantTradeoff.IsEmpty())
	{
		Out.Variant = Action.VariantName.IsEmpty() ? Action.VariantTradeoff
			: FText::Format(NSLOCTEXT("RTHud", "TooltipVariant", "{0}: {1}"), Action.VariantName, Action.VariantTradeoff);
	}
	return Out;
}

FRTActionTooltipView URTHudViewModel::BuildActionTooltip(const FRTAbilityCooldownView& Action, bool bArmed)
{
	// Titolo, frase, righe e variante vengono dal solo catalogo (#3748); qui si somma cio' che dipende dalla partita.
	FRTActionTooltipView Out = BuildCatalogTooltip(Action);
	if (Action.ActionId.IsNone())
	{
		return Out; // una posizione di kit vuota non ha niente da spiegare
	}
	Out.State = ResolveSlotState(Action, bArmed);

	// ── Il motivo di uno stato spento: uno solo, nell'ordine della definizione tecnica (D005) ────────────
	if (Action.bPlanInvalid)
	{
		Out.Reason = (Action.PlanInvalidReason == ERTActionInvalidReason::None)
			? NSLOCTEXT("RTHud", "TooltipPlanInvalid", "Piano illegale")
			: FText::Format(NSLOCTEXT("RTHud", "TooltipPlanInvalidBecause", "Piano illegale: {0}"),
				FText::FromString(URTTurnLogLibrary::DescribeInvalidReason(Action.PlanInvalidReason)));
	}
	else if (Action.bPlanDegraded && Action.PlanDegradedRefusal != ERTTargetRefusal::None
		&& Action.PlanDegradedRefusal != ERTTargetRefusal::Nothing)
	{
		Out.Reason = FText::Format(NSLOCTEXT("RTHud", "TooltipDegraded", "{0} — in risoluzione prendera' il ripiego"),
			FText::FromString(ARTHUD::RefusalText(Action.PlanDegradedRefusal, Action.PlanDegradedRange)));
	}
	else if (Action.TurnsRemaining > 0)
	{
		Out.Reason = FText::Format(NSLOCTEXT("RTHud", "TooltipCoolingDown", "In ricarica: ancora {0}"),
			TooltipCount(Action.TurnsRemaining, NSLOCTEXT("RTHud", "TooltipTurn", "turno"), NSLOCTEXT("RTHud", "TooltipTurns", "turni")));
	}
	return Out;
}

bool URTHudViewModel::SameTooltip(const FRTActionTooltipView& A, const FRTActionTooltipView& B)
{
	if (A.State != B.State || !A.Title.EqualTo(B.Title) || !A.Description.EqualTo(B.Description)
		|| !A.Reason.EqualTo(B.Reason) || !A.Variant.EqualTo(B.Variant) || A.Lines.Num() != B.Lines.Num())
	{
		return false;
	}
	for (int32 I = 0; I < A.Lines.Num(); ++I)
	{
		if (!A.Lines[I].Label.EqualTo(B.Lines[I].Label) || !A.Lines[I].Value.EqualTo(B.Lines[I].Value))
		{
			return false;
		}
	}
	return true;
}

FText URTHudViewModel::ComposeTooltipText(const FRTActionTooltipView& Tooltip)
{
	if (!Tooltip.IsValid())
	{
		return FText::GetEmpty();
	}
	TArray<FString> Righe;
	Righe.Add(Tooltip.Title.ToString());
	if (!Tooltip.Description.IsEmpty())
	{
		Righe.Add(Tooltip.Description.ToString());
	}
	if (!Tooltip.Variant.IsEmpty())
	{
		Righe.Add(Tooltip.Variant.ToString());
	}
	for (const FRTActionTooltipLine& Line : Tooltip.Lines)
	{
		Righe.Add(FString::Printf(TEXT("%s  %s"), *Line.Label.ToString(), *Line.Value.ToString()));
	}
	if (!Tooltip.Reason.IsEmpty())
	{
		Righe.Add(Tooltip.Reason.ToString());
	}
	return FText::FromString(FString::Join(Righe, TEXT("\n")));
}

FRTAbilityCooldownView URTHudViewModel::BuildCatalogActionRow(const URTActionData* Action, int32 Index, FName ActiveVariantId)
{
	FRTAbilityCooldownView View;
	View.AbilityIndex = Index;
	if (!Action)
	{
		return View; // una posizione vuota: `ActionId` resta `None`
	}
	View.ActionId = Action->Def.ActionId;
	// Le due chiavi si derivano QUI, dove il `Def` completo esiste.
	View.IconId = URTIconLibrary::MakeActionIconId(Action->Def.ActionId);
	View.FallbackIconId = URTIconLibrary::MakeActionIconFallbackId(Action->Def);
	View.DisplayName = Action->DisplayName;
	View.Slot = Action->Def.Slot;

	// La fase si LEGGE dal catalogo (`#3465`): `Phase` e' la risposta onesta, `PhaseMark` cio' che lo slot mostra.
	View.Phase = URTCatalogLibrary::MapResolutionPhase(Action->Def.ResolutionPhase);
	View.PhaseMark = PhaseMarkFor(Action->Def);
	View.PhaseLabel = PhaseMarkLabel(View.PhaseMark);

	// Il gruppo di lettura si deriva dallo stesso `Def` (`#3468`, D-455).
	View.Group = GroupFor(Action->Def);

	// Il denominatore della ricarica, mai negativo; la divisione la fa chi ha la ricarica vera.
	View.TotalTurns = FMath::Max(0, Action->Def.CooldownTurns);

	// `#3499`: la frase e i numeri del tooltip.
	View.Description = Action->Description;
	View.CooldownTurns = FMath::Max(0, Action->Def.CooldownTurns);
	// #3419: la forma e la variante attiva su QUEST'azione (`FindVariant`, non «la prima variante»).
	View.Shape = Action->Shape;
	View.AreaRadius = FMath::Max(0, Action->AreaRadius);
	if (const FRTAbilityVariant* Variante = Action->FindVariant(ActiveVariantId))
	{
		View.VariantName = Variante->DisplayName;
		View.VariantTradeoff = Variante->Tradeoff;
	}
	View.RangeCells = FMath::Max(0, Action->RangeCells);
	View.bSelfTarget = Action->bSelfTarget || Action->Def.bSelfTarget;
	for (const FRTActionEffectSpec& Effetto : Action->Def.Effects)
	{
		if (Effetto.Effect == ERTActionEffect::Damage)
		{
			View.Damage = FMath::Max(0, Effetto.Amount);
			break;
		}
	}
	return View;
}

TArray<FRTAbilityCooldownView> URTHudViewModel::BuildAbilityCooldowns(const ARTUnit* Unit)
{
	TArray<FRTAbilityCooldownView> Cooldowns;
	if (!Unit)
	{
		return Cooldowns;
	}

	for (int32 Index = 0; Index < Unit->NumAbilities(); ++Index)
	{
		const URTActionData* Action = Unit->GetAbility(Index);

		// 🔴 **Una posizione vuota produce una RIGA, non un salto** (`#2987`). Il `continue` che stava qui
		// accorciava l'array, e la posizione visiva si scollava da quella di kit — mentre il tasto continua
		// a significare *«la posizione N»*: `OnAbility6` chiama `SelectAbilityForCurrent(5)` comunque.
		// Saltare rendeva il sesto riquadro e il tasto `6` due cose diverse, senza che niente lo dicesse.
		//
		// ⚠️ **E rendeva VACUA l'asserzione che la difende**: `CooldownsMirrorTheSimulator` chiede
		// `Num() == NumAbilities()` e `[i].AbilityIndex == i`, e gira su un kit senza buchi — cioe' pinnava
		// il contratto esattamente nel caso in cui era gia' vero.
		//
		// 🔑 Il segnaposto porta **l'indice e il tasto**: sono le due cose vere anche di una posizione
		// vuota — il tasto la preme lo stesso, e `SelectAbilityForCurrent` la rifiuta da li'. `ActionId`
		// resta `None`, ed e' cio' da cui chi disegna riconosce il vuoto.
		if (!Action)
		{
			FRTAbilityCooldownView Empty;
			Empty.AbilityIndex = Index;
			Empty.HotkeyLabel = ARTPlayerController::HotkeyLabelFor(NAME_None, Index); // vuota non ha ActionId
			// ⛔ `ChargeFraction` resta al suo default `1.f`, che per un'azione dichiara «pronta». Qui non
			// significa nulla — non c'e' un'azione — e il campo che risponde e' `ActionId`. Scriverci `0`
			// direbbe «scarica», cioe' inventerebbe una ricarica per qualcosa che non ne ha una.
			//
			// Stessa ragione per la fase (`#3465`): `PhaseMark` resta `None` e `PhaseLabel` resta VUOTA, non
			// `—`. Il trattino dice «c'e' un'azione e non si gioca in nessuna fase», che di un vuoto e' falso.
			Cooldowns.Add(Empty);
			continue;
		}

		// La riga di catalogo viene dal catalogo (#3748); qui si somma cio' che la partita sa dell'unita'.
		FRTAbilityCooldownView View = BuildCatalogActionRow(Action, Index, Unit->ActiveVariantId);
		// Il tasto si LEGGE dalla tabella che la bindatura percorre, e non si calcola da `Index` (`#2987`).
		View.HotkeyLabel = ARTPlayerController::HotkeyLabelFor(Action->Def.ActionId, Index);
		// 🔑 **Tutti e TRE i campi del piano** (`#2988`): la principale, la reazione e lo scatto.
		View.bPlanned = (Unit->PlannedAbilityIndex == Index)
			|| (Unit->PlannedReactionAbility == Index)
			|| (Unit->PlannedDashAbility == Index);

		// Il numero si LEGGE dal simulatore, mai negativo (contratto della vista).
		View.TurnsRemaining = FMath::Max(0, Unit->GetAbilityCooldown(Index));

		// Il denominatore (`View.TotalTurns`) viene dal catalogo; la frazione si calcola qui, una volta (`#1896`).
		// Senza ricarica -> `1` (pronta per definizione); `Clamp` perche' la vista dichiara `[0,1]`.
		View.ChargeFraction = (View.TotalTurns > 0)
			? FMath::Clamp(1.f - static_cast<float>(View.TurnsRemaining) / static_cast<float>(View.TotalTurns), 0.f, 1.f)
			: 1.f;

		// Usabile ADESSO: da `D-324` guarda la sola ricarica.
		View.bUsableNow = Unit->CanUseAbility(Index);

		Cooldowns.Add(View);
	}

	// --- [D-459] lettura B: il piano dell'unita', letto due volte ---------------------------------------------
	// ⛔ I chiamanti sono due, ed entrambi restano nella PROPRIA squadra: la dock con `GetSelectedUnit()`, e
	// `BuildIdleBar` ([D-460]) sulle unita' della squadra di chi guarda, che di questa riga NON copia niente —
	// il suo elenco positivo dei campi lascia al default ogni stato di piano.
	//
	// 1) **Illegale -> `Invalid`** sullo slot della colpevole. Il validatore non legge l'unita' (D-190): un
	//    `FRTHexSimUnit` vuoto e' cio' che la firma chiede, non un'approssimazione.
	const FRTPlanValidation Verdetto =
		URTPlanValidationLibrary::ValidatePlan(FRTHexSimUnit(), URTPlanValidationLibrary::MakePlanFor(Unit));
	if (!Verdetto.bLegal && !Verdetto.OffendingActionId.IsNone())
	{
		for (FRTAbilityCooldownView& V : Cooldowns)
		{
			if (V.ActionId == Verdetto.OffendingActionId)
			{
				V.bPlanInvalid = true;
				V.PlanInvalidReason = Verdetto.Reason; // `#3499`: il perche', per il tooltip
			}
		}
	}

	// 2) **Degradato -> `Warning`** sullo slot della principale pianificata su un'unita': la domanda del click,
	//    con lo stesso nome (`RefusalForKnownTarget`). Un bersaglio ignoto da' `Nothing` e NON accende niente:
	//    il Warning su un'ombra direbbe «non lo vedi piu'» in un modo nuovo.
	//
	//    🔴 **Da DOVE si colpira', non da dove si sta.** La fase Blast parte dalla cella dello scatto, se lo
	//    scatto si applica (ordine `Prep -> Dash -> Blast -> Move`). ⏱️ *Fino a #3509 il click giudicava dalla
	//    cella corrente*, e il Warning nasceva da quella divergenza. Con [D-464] click e Warning chiedono la
	//    stessa origine per fase (`ARTUnit::AimOriginFor`): il Warning resta per il solo caso che [D-464] (4)
	//    gli lascia — lo scatto CAMBIATO dopo che il bersaglio era stato dichiarato. In planning nessuno si
	//    muove, ed e' lo scatto a separare «accettato» da «degradato».
	if (Cooldowns.IsValidIndex(Unit->PlannedAbilityIndex) && !Unit->bAttackTargetsCell)
	{
		const URTActionData* Pianificata = Unit->GetAbility(Unit->PlannedAbilityIndex);
		const ARTUnit* Bersaglio = Unit->PlannedAttackTarget.Get();
		FVector Origine; float Lato; float AltezzaPiano;
		const ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(Unit->GetWorld());
		const URTHexMapAsset* Mappa = HexMap ? HexMap->GetHexContext(Origine, Lato, AltezzaPiano) : nullptr;
		// [D-465] (3), `#2794`: lo stato Warning e' la stessa domanda del click, sulla stessa mappa.
		Mappa = ARTTurnManager::PlayerAimMap(Unit->GetWorld(), Unit->TeamId, Mappa);
		if (Pianificata && Bersaglio && Mappa)
		{
			// La regola e' `AimOriginCell`: qui se ne chiede la risposta per la fase dell'azione PIANIFICATA.
			const FRTCellId Da = Unit->AimOriginFor(Pianificata->Def.ResolutionPhase);

			const ERTTargetRefusal Rifiuto = URTCombatLibrary::RefusalForKnownTarget(Mappa, Da,
				Bersaglio->Cell, Pianificata->RangeCells, Pianificata->Def.LineOfSightPolicy,
				Bersaglio->IsKnownToObserver(), Pianificata->Def.MinRangeCells,
				URTCombatLibrary::CrossLayerAimFor(Pianificata->Def));
			Cooldowns[Unit->PlannedAbilityIndex].bPlanDegraded =
				Rifiuto != ERTTargetRefusal::None && Rifiuto != ERTTargetRefusal::Nothing;
			// `#3499`: il rifiuto per il tooltip, dalla stessa domanda e solo quando accende lo stato. ⛔ Un ignoto
			// e' gia' `Nothing`: il tooltip non puo' dire niente che lo stato non dica.
			if (Cooldowns[Unit->PlannedAbilityIndex].bPlanDegraded)
			{
				Cooldowns[Unit->PlannedAbilityIndex].PlanDegradedRefusal = Rifiuto;
				Cooldowns[Unit->PlannedAbilityIndex].PlanDegradedRange = (Rifiuto == ERTTargetRefusal::Range)
					? URTTerrainLibrary::EffectiveTargetingRange(Mappa, Da, Bersaglio->Cell, Pianificata->RangeCells)
					: INDEX_NONE;
			}
		}
	}

	return Cooldowns;
}

ERTActionPhaseMark URTHudViewModel::PhaseMarkFor(const FRTActionDef& Def)
{
	// 🔑 **Lo slot PRIMA della fase**, ed e' l'ordine che i due casi decisi il 2026-10-04 richiedono: una
	// reazione ha una `ResolutionPhase` — quella della sua core — e leggerla per prima direbbe `BLAST` di
	// qualcosa che non si gioca nel Blast. Lo stesso vale per `Wait`, che risolve in `NormalMovement`.
	if (Def.Slot == ERTActionSlot::Reaction)
	{
		return ERTActionPhaseMark::Reaction;
	}
	if (Def.Slot == ERTActionSlot::None)
	{
		return ERTActionPhaseMark::None;
	}

	// Funzione TOTALE sulla macro-fase, come `MapResolutionPhase`: nessun `default`, cosi' una fase aggiunta a
	// `ERTMatchPhase` senza un segno diventa un avviso di compilazione invece di uno slot muto.
	switch (URTCatalogLibrary::MapResolutionPhase(Def.ResolutionPhase))
	{
	case ERTMatchPhase::Prep:       return ERTActionPhaseMark::Prep;
	case ERTMatchPhase::Dash:       return ERTActionPhaseMark::Dash;
	case ERTMatchPhase::Blast:      return ERTActionPhaseMark::Blast;
	case ERTMatchPhase::Move:       return ERTActionPhaseMark::Move;
	case ERTMatchPhase::Cleanup:    return ERTActionPhaseMark::Cleanup;
	case ERTMatchPhase::Planning:   return ERTActionPhaseMark::None; // nessuna azione risolve nel Planning
	case ERTMatchPhase::MatchEnded: return ERTActionPhaseMark::None;
	}
	return ERTActionPhaseMark::None;
}

FText URTHudViewModel::PhaseMarkLabel(ERTActionPhaseMark Mark)
{
	// Le etichette del mockup della skill bar (`docs/research/design/hud/skill-bar-2026-10/`), in maiuscolo
	// come le stampa la striscia. `REAZ.` e' abbreviata perche' lo slot e' largo un'icona.
	switch (Mark)
	{
	case ERTActionPhaseMark::Prep:     return NSLOCTEXT("RTHud", "PhaseMarkPrep", "PREP");
	case ERTActionPhaseMark::Dash:     return NSLOCTEXT("RTHud", "PhaseMarkDash", "DASH");
	case ERTActionPhaseMark::Blast:    return NSLOCTEXT("RTHud", "PhaseMarkBlast", "BLAST");
	case ERTActionPhaseMark::Move:     return NSLOCTEXT("RTHud", "PhaseMarkMove", "MOVE");
	case ERTActionPhaseMark::Cleanup:  return NSLOCTEXT("RTHud", "PhaseMarkCleanup", "CLEANUP");
	case ERTActionPhaseMark::Reaction: return NSLOCTEXT("RTHud", "PhaseMarkReaction", "REAZ.");
	case ERTActionPhaseMark::None:     return NSLOCTEXT("RTHud", "PhaseMarkNone", "—");
	}
	return NSLOCTEXT("RTHud", "PhaseMarkNone", "—");
}

ERTActionGroup URTHudViewModel::GroupFor(const FRTActionDef& Def)
{
	// 🔑 **Le generiche PRIMA dell'attacco base**: nessuna delle due condizioni oggi ruba un'azione all'altra
	// — `Action.BasicAttack` non e' fra le generiche che entrano nel kit — ma l'ordine e' quello della regola di
	// D-455, e scriverlo uguale evita che una lettura del codice ne deduca un'altra.
	if (URTCatalogLibrary::GetGenericActionIds().Contains(Def.ActionId))
	{
		return ERTActionGroup::Common;
	}

	static const FName BasicAttackId(TEXT("Action.BasicAttack"));
	if (Def.ActionId == BasicAttackId || Def.BaseActionId == BasicAttackId)
	{
		return ERTActionGroup::Base;
	}

	return ERTActionGroup::Kit;
}

FRTMovementReadoutView URTHudViewModel::BuildMovementReadout(const ARTUnit* Unit)
{
	FRTMovementReadoutView View;
	if (!Unit)
	{
		return View;
	}

	const FRTUnitSlotsView Slots = BuildUnitSlots(Unit);
	View.bAuthorized = Slots.bAuthorized;
	View.ProfileId = Slots.MovementProfileId;
	View.Label = MovementReadoutLabel(View.ProfileId);
	// `PlannedMovementProfileId` porta SOLO la dichiarazione del giocatore (oggi `Sneak`, o vuoto): [D-425].
	View.bSneakDeclared = Unit->PlannedMovementProfileId == URTMovementProfileLibrary::ProfileSneak;
	View.SneakKeyLabel = ARTPlayerController::SneakHotkey().GetDisplayName(/*bLongDisplayName=*/ false);
	return View;
}

FText URTHudViewModel::MovementReadoutLabel(FName ProfileId)
{
	using Lib = URTMovementProfileLibrary;

	const FRTMovementProfile Profile = Lib::FindProfile(ProfileId);
	if (ProfileId.IsNone() || Profile.Id != ProfileId)
	{
		return FText::GetEmpty(); // un id che il catalogo non conosce non ha un'etichetta da inventare
	}

	// Il nome canonico e' l'ultimo segmento dell'id — `MovementProfile.Sprint` -> `Sprint` — cosi' un profilo
	// nuovo ha gia' un nome invece di un'etichetta vuota.
	FString Nome = ProfileId.ToString();
	int32 Punto = INDEX_NONE;
	if (Nome.FindLastChar(TEXT('.'), Punto))
	{
		Nome = Nome.RightChop(Punto + 1);
	}

	// Chi sta fermo non ha un passo da moltiplicare: il numero mentirebbe su un budget che non si spende.
	if (ProfileId == Lib::ProfileStill)
	{
		return FText::FromString(Nome);
	}

	// Il moltiplicatore in centesimi, scritto all'italiana: `25` -> `0,25`, `50` -> `0,5`, `200` -> `2`.
	const int32 Percento = FMath::Max(0, Profile.MoveBudgetPercent);
	FString Moltiplicatore = FString::FromInt(Percento / 100);
	const int32 Resto = Percento % 100;
	if (Resto != 0)
	{
		FString Decimali = FString::Printf(TEXT("%02d"), Resto);
		Decimali.RemoveFromEnd(TEXT("0"));
		Moltiplicatore += TEXT(",") + Decimali;
	}
	return FText::FromString(FString::Printf(TEXT("%s \u00D7%s"), *Nome, *Moltiplicatore));
}

TArray<FRTAbilityCooldownView> URTHudViewModel::OrderForReading(const TArray<FRTAbilityCooldownView>& Actions)
{
	// Il rango di lettura di un gruppo. Funzione TOTALE sull'enum, senza `default`: un gruppo aggiunto senza
	// rango diventa un avviso di compilazione invece di una voce che finisce in coda per caso.
	auto Rango = [](ERTActionGroup Group) -> int32
	{
		switch (Group)
		{
		case ERTActionGroup::Common: return 0;
		case ERTActionGroup::Base:   return 1;
		case ERTActionGroup::Kit:    return 2;
		case ERTActionGroup::None:   return 2; // posizione vuota del kit: si legge col Kit, al suo posto
		}
		return 2;
	};

	// ⚠️ **`StableSort` e non `Sort`**: dentro un gruppo tutte le voci hanno lo stesso rango, e senza
	// stabilita' l'ordine di kit — cioe' quello in cui il giocatore impara i tasti — dipenderebbe
	// dall'algoritmo. `HudViewModel.ReadingOrderKeepsKitOrderWithinEachGroup` lo pinna.
	TArray<FRTAbilityCooldownView> Ordinate = Actions;
	Ordinate.StableSort([&Rango](const FRTAbilityCooldownView& A, const FRTAbilityCooldownView& B)
	{
		return Rango(A.Group) < Rango(B.Group);
	});

	// Il confine si misura sul RANGO, non su `Group`: una posizione vuota (`None`) si legge col Kit, e fra le
	// due non c'e' un separatore (#3489).
	for (int32 i = 0; i < Ordinate.Num(); ++i)
	{
		Ordinate[i].bGroupBreakBefore = i > 0 && Rango(Ordinate[i].Group) != Rango(Ordinate[i - 1].Group);
		Ordinate[i].bFirstOfGroup = i == 0 || Ordinate[i].bGroupBreakBefore;
	}
	return Ordinate;
}

TArray<FRTAbilityCooldownView> URTHudViewModel::BuildIdleBar(const TArray<const ARTUnit*>& OwnUnits)
{
	TArray<FRTAbilityCooldownView> Comuni;
	int32 KitPiuLungo = 0;
	for (const ARTUnit* Unit : OwnUnits)
	{
		if (!Unit)
		{
			continue;
		}
		const TArray<FRTAbilityCooldownView> Righe = OrderForReading(BuildAbilityCooldowns(Unit));
		int32 Kit = 0;
		for (const FRTAbilityCooldownView& Riga : Righe)
		{
			// Una posizione vuota si legge col Kit, come in `OrderForReading`.
			Kit += (Riga.Group == ERTActionGroup::Kit || Riga.Group == ERTActionGroup::None) ? 1 : 0;
		}
		KitPiuLungo = FMath::Max(KitPiuLungo, Kit);

		// Le Comuni dalla prima unita' dell'elenco: sono le stesse per ogni eroe, e l'elenco arriva gia'
		// ordinato (`GatherUnitsInWorld`), quindi la fonte non cambia fra due frame.
		if (Comuni.IsEmpty())
		{
			for (const FRTAbilityCooldownView& Riga : Righe)
			{
				if (Riga.Group != ERTActionGroup::Common)
				{
					continue;
				}
				// Elenco POSITIVO dei campi: cio' che non e' qui resta al default, quindi nessun piano,
				// ricarica o stato di D-459 passa dall'unita' alla struttura.
				FRTAbilityCooldownView Spenta;
				Spenta.ActionId = Riga.ActionId;
				Spenta.IconId = Riga.IconId;
				Spenta.FallbackIconId = Riga.FallbackIconId;
				Spenta.DisplayName = Riga.DisplayName;
				Spenta.HotkeyLabel = Riga.HotkeyLabel;
				Spenta.Slot = Riga.Slot;
				Spenta.Phase = Riga.Phase;
				Spenta.PhaseMark = Riga.PhaseMark;
				Spenta.PhaseLabel = Riga.PhaseLabel;
				// `#3499`: i fatti di CATALOGO del tooltip, uguali per chiunque porti l'azione. ⛔ Nessuno stato di
				// piano: `TotalTurns`, `TurnsRemaining` e i motivi restano al default, come [D-460] chiede.
				Spenta.Description = Riga.Description;
				Spenta.RangeCells = Riga.RangeCells;
				Spenta.bSelfTarget = Riga.bSelfTarget;
				Spenta.Damage = Riga.Damage;
				Spenta.CooldownTurns = Riga.CooldownTurns;
				Spenta.Shape = Riga.Shape;
				Spenta.AreaRadius = Riga.AreaRadius;
				Spenta.Group = ERTActionGroup::Common;
				Spenta.AbilityIndex = IdleSlotIndex;
				Spenta.bUsableNow = false;
				Comuni.Add(Spenta);
			}
		}
	}

	TArray<FRTAbilityCooldownView> Struttura = Comuni;
	if (OwnUnits.ContainsByPredicate([](const ARTUnit* U) { return U != nullptr; }))
	{
		FRTAbilityCooldownView Vuoto;
		Vuoto.AbilityIndex = IdleSlotIndex;
		Vuoto.Group = ERTActionGroup::Base;
		Struttura.Add(Vuoto);
		Vuoto.Group = ERTActionGroup::Kit;
		for (int32 i = 0; i < KitPiuLungo; ++i)
		{
			Struttura.Add(Vuoto);
		}
	}
	// Gia' in ordine: `OrderForReading` scrive i confini dei gruppi, come per una barra con un'unita'.
	return OrderForReading(Struttura);
}

TArray<FRTUnitCardView> URTHudViewModel::BuildTeamRoster(const TArray<ARTUnit*>& Units, int32 PlayerTeamId)
{
	TArray<FRTUnitCardView> Roster;
	for (const ARTUnit* Unit : Units)
	{
		// Le morte restano: `bAlive` lo dice, e un elenco che si accorcia fa perdere il conto della squadra.
		if (Unit && Unit->TeamId == PlayerTeamId)
		{
			Roster.Add(BuildUnitCard(Unit, PlayerTeamId));
		}
	}
	return Roster;
}

FRTDamageTokenView URTHudViewModel::BuildDamageToken(int32 Amount, int32 TargetStableUnitId)
{
	FRTDamageTokenView View;

	// ⚠️ `0` e' «nessuno» ([D-063]), non l'unita' numero zero: si conserva com'e' invece di normalizzarlo a
	// `INDEX_NONE`, perche' e' la sentinella che `FRTResolvedEvent` gia' usa e tradurla creerebbe due
	// vocabolari per lo stesso fatto. Chi disegna confronta con lo `StableUnitId` della propria unita': uno
	// zero non combacia con nessuno, ed e' esattamente l'effetto voluto.
	View.TargetStableUnitId = TargetStableUnitId;
	View.Amount = Amount;

	// 🔴 **La soglia e' `<= 0`, non `== 0`.** Un valore negativo non e' atteso — `Hit.Power` passa da
	// `FMath::Max(0, ...)` — ma se arrivasse sarebbe un difetto del simulatore, e `-(-3)` stamperebbe `+3`
	// sopra la testa di chi ha appena incassato: una cifra plausibile e falsa. Il ramo dello zero e' il
	// posto giusto dove farlo cadere, perche' dice «nessuna quantita' da mostrare» senza inventarne una.
	View.bHasDamage = (Amount > 0);
	View.bPulsesHealthBar = View.bHasDamage; // #3600: la barra pulsa se la vita e' cambiata

	if (View.bHasDamage)
	{
		// Il segno fa parte del canale: `#2453` vieta il colore come UNICO canale, e una cifra nuda si
		// leggerebbe come una cura tanto quanto come un danno. Il meno lo disambigua senza dipendere dalla
		// tinta.
		View.Label = FString::Printf(TEXT("-%d"), Amount);
	}
	else
	{
		// L'uscita (a) di `#2455`. Dice **quanto** — zero — e tace sul **perche'**, che il dato non porta.
		// ⛔ Non e' la cifra nuda `0`: quella si legge come una quantita' di danno inflitta, ed e' cio' che
		// l'AC esclude. La parola accanto la rende una dichiarazione invece di un numero.
		View.Label = TEXT("0 danni");
	}

	return View;
}

FRTDamageTokenView URTHudViewModel::BuildHealToken(int32 Amount, int32 TargetStableUnitId)
{
	FRTDamageTokenView View;
	View.TargetStableUnitId = TargetStableUnitId;
	View.Amount = Amount;
	View.bIsHeal = true;
	// ⛔ Una cura non e' un danno: `bHasDamage` resta falso, e chi lo legge per contare i colpi non conta questa.
	View.bHasDamage = false;
	View.bPulsesHealthBar = (Amount > 0);
	// Il segno fa parte del canale, come il `-` del colpo (`#2453`): una cifra nuda si leggerebbe come un danno
	// tanto quanto come una cura. A salute piena la cura restituisce zero, e lo zero si DICHIARA con una parola,
	// come il «0 danni» del colpo: un «+0» si leggerebbe come una quantita'.
	View.Label = (Amount > 0) ? FString::Printf(TEXT("+%d"), Amount) : FString(TEXT("0 curati"));
	return View;
}

TArray<FRTPlannedIntent> URTHudViewModel::BuildAuthoritativeIntents(const TArray<AActor*>& Actors)
{
	TArray<FRTPlannedIntent> Authoritative;
	Authoritative.Reserve(Actors.Num());
	for (AActor* Actor : Actors)
	{
		const ARTUnit* Unit = Cast<ARTUnit>(Actor);
		if (!Unit || !Unit->IsAlive()) { continue; }

		const URTActionData* Planned = Unit->GetAbility(Unit->PlannedAbilityIndex);
		const URTActionData* Reaction = Unit->GetAbility(Unit->PlannedReactionAbility);

		FRTPlannedIntent Intent;
		Intent.OwnerCell = Unit->Cell;
		Intent.TeamId = Unit->TeamId;
		Intent.bAlive = true;
		Intent.bRevealed = Unit->HasStatus(TAG_Status_Reveal);
		Intent.bMoving = Unit->HasPlannedNormalMove(); // la regola intera, non la sua meta'
		Intent.PlannedCell = Unit->PlannedCell;
		Intent.ActionName = Planned ? Planned->DisplayName : FText::GetEmpty();
		Intent.bHasTarget = (Unit->PlannedAttackTarget != nullptr && Unit->PlannedAttackTarget->IsAlive());
		Intent.TargetCell = Intent.bHasTarget ? Unit->PlannedAttackTarget->Cell : Unit->Cell;
		// ⚖️ **Lo slot `Reaction`, non `URTCatalogLibrary::ArmsReaction`** (`#3628`, decisione d'autore del
		// 2026-10-11). `ReactionName` finisce nell'etichetta dell'intento come «(reazione: … ?)», e l'Overwatch come
		// principale e' gia' `ActionName`: contarlo qui stamperebbe lo stesso nome due volte. Il roster, che un nome
		// d'azione non lo mostra, lo conta ([D-478]).
		Intent.ReactionName = Reaction ? Reaction->DisplayName : FText::GetEmpty();
		Intent.PlannedPath = Unit->PlannedPath;
		Intent.PlannedWaypoints = Unit->PlannedWaypoints;
		Intent.bDashing = (Unit->PlannedDashAbility != INDEX_NONE);
		Intent.DashCell = Unit->PlannedDashCell;
		// Rotazione dichiarata (D-020, #291): finora `bDeclaresRotation` e `DeclaredFacing` restavano ai
		// default perche' nessuno li valorizzava, e `FilterForTeam` filtrava un campo sempre vuoto.
		Intent.bDeclaresRotation = Unit->bDeclaresPlannedFacing;
		Intent.DeclaredFacing = Unit->PlannedFacing;
		if (const URTActionData* DashAb = Unit->GetAbility(Unit->PlannedDashAbility))
		{
			Intent.DashStyle = DashAb->Def.MovementStyle;
		}
		Intent.DashReachCells = Unit->PlannedChargeReachBeyondRange(); // [D-516]
		// [D-528]: dove si ferma DAVVERO la carica, come l'ha letta chi l'ha pianificata con la conoscenza della sua squadra.
		// Senza lettura (un bot, un piano scritto senza click) resta `INDEX_NONE` e l'anteprima ricade su `DashReachCells`.
		if (FRTChargeRun Corsa; Unit->GetPlannedChargeRun(Corsa))
		{
			Intent.DashRunCells = Corsa.RunCells;
			Intent.bDashMissesTarget = Corsa.bMissesTarget;
		}
		Authoritative.Add(Intent);
	}
	return Authoritative;
}

bool URTHudViewModel::DeclaresRoundLimit(const FRTMatchHeaderView& Header)
{
	return Header.RoundLimit > 0;
}

FString URTHudViewModel::ComposeRoundCounter(const FRTMatchHeaderView& Header)
{
	// Il limite si nomina solo se il formato ne dichiara uno: lo dice `DeclaresRoundLimit`, la stessa condizione che
	// `RefreshLook` applica alla porta `RoundLimitText`.
	return DeclaresRoundLimit(Header)
		? FString::Printf(TEXT("Round %d/%d"), Header.Round, Header.RoundLimit)
		: FString::Printf(TEXT("Round %d"), Header.Round);
}


#define LOCTEXT_NAMESPACE "RTPlayerEventFeed"

FText URTHudViewModel::ComposePlayerEventText(const FRTPlayerEvent& Event)
{
	switch (Event.Type)
	{
	case ERTPlayerEventType::AttackBlocked:
		// 🔴 **La frase non nomina la cella, e il DOVE non e' perduto**: viaggia in `BlockerCell`, che chi
		// disegna marca nel mondo. E' la meta' che mancava al verdetto d'autore — *«non ci sono riferimenti
		// video, solo log»* — e stampare `(q=0,r=0,L=0)` l'avrebbe lasciata scoperta comunque, perche' un
		// giocatore non converte coordinate assiali guardando lo schermo.
		return LOCTEXT("AttackBlocked", "Nessuna linea di tiro");

	case ERTPlayerEventType::MoveBlocked:   return LOCTEXT("MoveBlocked", "Movimento bloccato");
	case ERTPlayerEventType::Moved:         return LOCTEXT("Moved", "Spostata");
	case ERTPlayerEventType::SlideBlocked:  return LOCTEXT("SlideBlocked", "Il terreno non l'ha spostata");
	case ERTPlayerEventType::Defeated:      return LOCTEXT("Defeated", "Eliminata");
	case ERTPlayerEventType::ReactionFired: return LOCTEXT("ReactionFired", "Reazione scattata");
	case ERTPlayerEventType::StatusChanged: return LOCTEXT("StatusChanged", "Stato cambiato");
	case ERTPlayerEventType::Environment:   return LOCTEXT("Environment", "L'ambiente cambia");
	case ERTPlayerEventType::ObjectiveChanged: return LOCTEXT("Objective", "Obiettivo aggiornato");

	case ERTPlayerEventType::Attacked:
	{
		// 🔑 **Un danno da CADUTA dice la propria causa** (`#3081`, decisione d'autore del 2026-10-13). Chi
		// cade ha nello stesso turno la voce di movimento E quella di danno, e la dominanza tiene una riga
		// per unita': vince il danno. Senza questo ramo quella riga direbbe «Colpita», e nel feed una caduta
		// non si distinguerebbe da un colpo — che e' cio' che la issue ha misurato.
		//
		// La causa viaggia gia' in `ActionId`, su una voce che `IsAuthorized` ha gia' esaminato: qui non si
		// ricalcola niente e non entra nessun campo nuovo. I due id sono di `URTTurnLogLibrary`, che li
		// condivide col produttore.
		//
		// ⚠️ **Due confronti fra `FName`, non un prefisso.** Il feed si ricompone a ogni fotogramma
		// (`ARTHUD::DrawHUD`), e un `ToString()` per riga allocherebbe una stringa per arrivare a «Colpita» sul
		// colpo ordinario. E una causa `Fall.*` che il produttore scrivesse domani non e' detto che parli di
		// chi cade: letta per prefisso diventerebbe «Caduta» su un'unita' rimasta ferma — lo stesso errore di
		// soggetto che ha reso necessario il ramo dell'urto. Un id nuovo si aggiunge QUI, accanto ai due.
		if (Event.ActionId == URTTurnLogLibrary::FallCauseOfImpact())
		{
			// Non e' caduta lei: e' l'occupante su cui qualcuno e' atterrato (`spec-caduta-e-bordi.md` §5.1).
			return FText::Format(LOCTEXT("FallImpact", "Urto — {0}"), FText::AsNumber(Event.Amount));
		}
		if (Event.ActionId == URTTurnLogLibrary::FallCauseOfFaller())
		{
			return FText::Format(LOCTEXT("Fell", "Caduta — {0}"), FText::AsNumber(Event.Amount));
		}
		return FText::Format(LOCTEXT("Attacked", "Colpita — {0}"), FText::AsNumber(Event.Amount));
	}
	case ERTPlayerEventType::Healed:
		return FText::Format(LOCTEXT("Healed", "Curata — {0}"), FText::AsNumber(Event.Amount));
	}

	// ⚠️ **Il `default` non e' irraggiungibile e non deve tacere.** Un valore nuovo dell'enum non tradotto
	// sopra arriverebbe qui: una riga generica e' un difetto visibile, una riga **vuota** e' una sparizione
	// che nessuno nota — lo stesso difetto che `SlideIsImportant` documenta un piano piu' in la'.
	return LOCTEXT("Unknown", "Qualcosa e' successo");
}

TArray<FRTPlayerEventLineView> URTHudViewModel::BuildPlayerEventFeed(const TArray<FRTTurnLogEntry>& TurnLog,
	int32 ObserverTeamId)
{
	return BuildPlayerEventFeed(TurnLog, TArray<int32>{ ObserverTeamId });
}

int32 URTHudViewModel::OwnTeamForFeed(const ARTTurnManager* TurnManager, const TArray<int32>& ObserverTeamIds)
{
	// Presidiata e una squadra sola che guarda: e' lei. Altrimenti nessuna, e la scala di sempre (`#3734`).
	return (TurnManager != nullptr && !TurnManager->IsUnattendedSession() && ObserverTeamIds.Num() == 1)
		? ObserverTeamIds[0]
		: INDEX_NONE;
}

TArray<FRTPlayerEventLineView> URTHudViewModel::BuildPlayerEventFeed(const TArray<FRTTurnLogEntry>& TurnLog,
	const TArray<int32>& ObserverTeamIds, int32 OwnTeamId)
{
	// 🔴 **Il PERIMETRO viene prima della proiezione, ed e' una correzione non un taglio.**
	// `Project` applica la dominanza — una riga per unita' — e il suo commento la descrive «in questo
	// turno». Passandogli la partita intera quella regola cambia significato: chi e' andato KO al round 3
	// tiene la riga fino alla fine, perche' nessun evento successivo ha rango piu' alto, e cio' che gli e'
	// accaduto dopo non si vede. Anche l'aggregazione dell'ambiente («quante celle, non quali») si
	// fonderebbe su dodici round in un contatore solo.
	int32 UltimoTurno = 0;
	for (const FRTTurnLogEntry& Entry : TurnLog)
	{
		UltimoTurno = FMath::Max(UltimoTurno, Entry.TurnNumber);
	}

	TArray<FRTTurnLogEntry> DelTurnoCorrente;
	DelTurnoCorrente.Reserve(TurnLog.Num());
	for (const FRTTurnLogEntry& Entry : TurnLog)
	{
		if (Entry.TurnNumber == UltimoTurno)
		{
			DelTurnoCorrente.Add(Entry);
		}
	}

	// ⛔ **L'autorizzazione non si ripete qui, e non deve.** `Project` la applica come primo passo, sul
	// verdetto che ogni voce porta congelato ([D-223]). Riapplicarla sarebbe un secondo contratto di
	// conoscenza — il difetto che `#1936` vieta — e ometterla sarebbe il leak. Questa funzione **compone** e
	// nient'altro.
	//
	// ⚠️ Il filtro qui sopra non tocca l'autorizzazione: seleziona QUALI voci entrano, non chi puo'
	// vederle. Le due decisioni restano separate, e quella di privacy resta una sola.
	const TArray<FRTPlayerEvent> Events = URTPlayerEventProjector::Project(DelTurnoCorrente, ObserverTeamIds, OwnTeamId);

	// Le **ultime** `MaxFeedLines`: il giocatore vuole sapere cos'e' appena successo, non come il turno
	// era cominciato. Prendere le prime sarebbe altrettanto implementabile e inutile.
	const int32 Prime = FMath::Max(0, Events.Num() - MaxFeedLines);

	TArray<FRTPlayerEventLineView> Lines;
	Lines.Reserve(Events.Num() - Prime);
	for (int32 i = Prime; i < Events.Num(); ++i)
	{
		const FRTPlayerEvent& Event = Events[i];
		FRTPlayerEventLineView& Line = Lines.AddDefaulted_GetRef();
		Line.Text = ComposePlayerEventText(Event);
		Line.Importance = Event.Importance;
		Line.PrimaryStableUnitId = Event.PrimaryUnitId;
		Line.BlockerCell = Event.BlockerCell;

		// ⚠️ La sentinella si legge **una volta**, qui, e diventa un `bool`. Lasciarla al widget
		// rimetterebbe nel Blueprint la distinzione fra «cella (0,0,0)» e «nessuna cella», che e' proprio
		// dove la cella `(0,0,0)` e' una cella vera e marcherebbe l'origine dell'arena.
		Line.bHasBlocker = Event.HasBlockerCell();
		Line.Phase = Event.Phase;
		Line.Sequence = i + 1; // la posizione fra gli eventi GIA' filtrati: nessun buco da cui contare il nascosto
		Line.bReaction = Event.Type == ERTPlayerEventType::ReactionFired;
	}
	return Lines;
}

TArray<FString> URTHudViewModel::DescribeFeedState(const ARTTurnManager* TurnManager,
	const TArray<int32>& ObserverTeamIds)
{
	TArray<FString> Righe;

	if (TurnManager == nullptr)
	{
		Righe.Add(TEXT("[RT] Feed: nessun TurnManager acquisito — il widget non ha contesto."));
		Righe.Add(TEXT("[RT]   NativeTick lo ricerca finche' non lo trova: se questa riga persiste a partita "
					   "avviata, l'acquisizione non e' avvenuta e il vuoto NON e' privacy."));
		return Righe;
	}

	const TArray<FRTTurnLogEntry>& Log = TurnManager->GetTurnLog();
	// La stessa regola del feed a schermo (`#3734`): senza, la diagnostica conterebbe righe diverse da quelle che
	// il widget mostra, e indicherebbe lo strato sbagliato.
	const TArray<FRTPlayerEventLineView> Filtrato =
		BuildPlayerEventFeed(Log, ObserverTeamIds, OwnTeamForFeed(TurnManager, ObserverTeamIds));

	FString Osservatori;
	for (const int32 Id : ObserverTeamIds)
	{
		Osservatori += (Osservatori.IsEmpty() ? TEXT("") : TEXT(", ")) + FString::FromInt(Id);
	}

	Righe.Add(FString::Printf(
		TEXT("[RT] Feed: TurnLog=%d voci, osservatori={%s}, righe dopo filtro=%d, budget=%d"),
		Log.Num(), Osservatori.IsEmpty() ? TEXT("vuoto") : *Osservatori, Filtrato.Num(), MaxFeedLines));

	// ⚠️ **Il TurnLog e' l'ULTIMO turno risolto, non la cronaca della partita**: `LockInAndResolve` lo azzera
	// al commit e lo ripopola durante le fasi. Zero voci PRIMA della prima risoluzione e' lo stato corretto,
	// e chiamarlo difetto manderebbe a cercare un guasto che non c'e'.
	if (Log.Num() == 0)
	{
		Righe.Add(TEXT("[RT]   Il TurnLog e' vuoto: nessun turno ancora risolto, oppure e' appena stato "
					   "azzerato dal commit. Non e' un difetto finche' una risoluzione non e' avvenuta."));
		return Righe;
	}

	// 🔴 Il caso che questa funzione esiste per nominare: il log ha voci, e a schermo non arriva niente.
	if (Filtrato.Num() == 0)
	{
		Righe.Add(TEXT("[RT]   Il log ha voci ma NESSUNA passa il filtro dell'osservatore."));
		Righe.Add(TEXT("[RT]   Se l'insieme degli osservatori qui sopra non e' quello atteso, la causa e' "
					   "ResolveObserverTeamIds (sessione presidiata o no), non il feed."));
		return Righe;
	}

	// ⛔ Se si arriva qui il ViewModel PRODUCE le righe: un vuoto a schermo e' a valle, nel widget che le
	// disegna. E' la distinzione che `#2964` ha dovuto ricostruire a mano.
	Righe.Add(FString::Printf(
		TEXT("[RT]   Il ViewModel produce %d righe: se a schermo non compaiono, il difetto e' nel widget."),
		Filtrato.Num()));
	for (int32 i = 0; i < Filtrato.Num(); ++i)
	{
		Righe.Add(FString::Printf(TEXT("[RT]     %d) %s"), i + 1, *Filtrato[i].Text.ToString()));
	}
	return Righe;
}

TArray<FRTPlayerEventLineView> URTHudViewModel::BuildPlayerEventFeed(const ARTTurnManager* TurnManager,
	int32 ObserverTeamId)
{
	// Nessun manager, nessun log: un feed vuoto e' gia' la risposta giusta, e non c'e' una «vista neutra»
	// da inventare come per l'intestazione — una riga non esiste finche' qualcosa non e' successo.
	// Per l'insieme di un elemento: e' li' che la regola di `#3734` sa se la sessione e' presidiata.
	return BuildPlayerEventFeed(TurnManager, TArray<int32>{ ObserverTeamId });
}

TArray<FRTPlayerEventLineView> URTHudViewModel::BuildPlayerEventFeed(const ARTTurnManager* TurnManager,
	const TArray<int32>& ObserverTeamIds)
{
	return TurnManager != nullptr
		? BuildPlayerEventFeed(TurnManager->GetTurnLog(), ObserverTeamIds, OwnTeamForFeed(TurnManager, ObserverTeamIds))
		: TArray<FRTPlayerEventLineView>{};
}

TArray<int32> URTHudViewModel::ResolveObserverTeamIds(const ARTTurnManager* TurnManager, int32 PlayerTeamId,
	const TArray<ARTUnit*>& Units)
{
	// ⛔ **Senza manager si risponde come una sessione presidiata, non come una non presidiata.** Il dubbio
	// si risolve verso la vista PIU' STRETTA: un widget nato prima del manager (`NativeTick` lo riacquisisce
	// dopo pochi frame) mostrerebbe altrimenti entrambe le squadre per quei frame, ed e' un leak temporale
	// invece di un difetto di layout.
	if (TurnManager == nullptr || !TurnManager->IsUnattendedSession())
	{
		return TArray<int32>{ PlayerTeamId };
	}

	// ── Sessione non presidiata: chi guarda non gioca in nessuna delle due squadre (`#2386`).
	//
	// 🔴 **Le squadre si SCOPRONO dalle unita' in campo, non da una costante `2`.** Un letterale reggerebbe
	// oggi e si romperebbe al primo formato 3v3 (`E24`, `#325`) mostrando due squadre su tre — e sarebbe un
	// difetto silenzioso, perche' il roster resterebbe pieno.
	TArray<int32> TeamIds;
	for (const ARTUnit* Unit : Units)
	{
		if (Unit)
		{
			TeamIds.AddUnique(Unit->TeamId);
		}
	}

	// La propria squadra c'e' comunque: senza unita' in campo — allestimento fallito, mondo di prova vuoto —
	// l'insieme sarebbe vuoto e il feed tacerebbe per una ragione che non e' l'autorizzazione.
	// ⚠️ **Prima del `Sort`**: aggiungerla dopo la rimetterebbe in coda e l'ordine dichiarato sarebbe falso
	// esattamente nel caso che questa riga esiste per coprire.
	TeamIds.AddUnique(PlayerTeamId);

	// ⚠️ **Ordinato, e non per estetica.** L'insieme decide l'ordine delle liste del roster e viaggia in una
	// vista: l'ordine di `GetAllActorsOfClass` non e' dichiarato, e due frame potrebbero scambiare le due
	// squadre di posto. E' la stessa disciplina per cui il roster ordina per `HeroId`.
	TeamIds.Sort();
	return TeamIds;
}

#undef LOCTEXT_NAMESPACE


#define LOCTEXT_NAMESPACE "RTTargetPrompt"

FRTTargetPromptView URTHudViewModel::BuildTargetPrompt(ERTPointerContext Context, ERTPointerTargetKind Kind)
{
	FRTTargetPromptView View;

	// 🔴 **Il CONTESTO viene prima della forma, e l'ordine e' quello che il contratto dichiara gia'.**
	// `GetPointerContext()` mette `Modal` e `ResolutionPlayback` davanti a ogni altro ramo — «con la pausa a
	// schermo nessun click deve raggiungere il mondo, qualunque cosa sia selezionata». Guardare prima il
	// `Kind` mostrerebbe «scegli un'unita'» a partita in pausa, con un'azione rimasta armata da prima.
	//
	// ⚠️ `ReactionWindow` sta qui pur non avendo ancora un produttore (e' E14), e NON e' il «campo senza
	// produttore» che questo repository evita: non e' uno stato nuovo che qualcuno dovrebbe scrivere, e'
	// un valore dell'enum del puntatore che va classificato comunque. Ometterlo lo farebbe cadere nel
	// `default` in fondo, cioe' su una risposta sbagliata invece che su una dichiarata.
	if (Context == ERTPointerContext::Modal
		|| Context == ERTPointerContext::ResolutionPlayback
		|| Context == ERTPointerContext::ReactionWindow)
	{
		View.Kind = ERTTargetPromptKind::NotApplicable;
		return View; // testo vuoto: e' l'unico caso in cui lo e', e `Kind` lo nomina.
	}

	if (Context == ERTPointerContext::IdleSelection)
	{
		View.Kind = ERTTargetPromptKind::NoSelection;
		View.Text = LOCTEXT("NoSelection", "Seleziona un'unita'");
		return View;
	}

	// La rotazione finale non passa da un `TargetKind`: e' un contesto suo, ed e' il quarto dei cinque
	// prompt che la DoD elenca.
	if (Context == ERTPointerContext::Facing)
	{
		View.Kind = ERTTargetPromptKind::ChooseFacing;
		View.Text = LOCTEXT("ChooseFacing", "Scegli l'orientamento");
		View.bIsAwaitingTarget = true;
		return View;
	}

	// `Planning` e `Pathing` sono entrambi «unita' selezionata, niente armato»: il neutro di D-128. Restano
	// due contesti perche' il puntatore ci fa cose diverse, ma la domanda posta al giocatore e' la stessa —
	// e' ancora «scegli un'azione».
	if (Context != ERTPointerContext::Targeting)
	{
		View.Kind = ERTTargetPromptKind::Neutral;
		View.Text = LOCTEXT("Neutral", "Scegli un'azione");
		return View;
	}

	// --- Da qui in giu' un'azione E' armata, e il `Kind` dice cosa chiede -----------------------------
	switch (Kind)
	{
	case ERTPointerTargetKind::None:
		// 🔑 **Questo `None` non e' un'assenza: e' una risposta.** L'azione e' armata e non ha niente da
		// puntare (`TargetKindForAction`: «un supporto su se stessi non ha niente da puntare, si pianifica
		// alla pressione del tasto»). Dirlo «nessuna azione» sarebbe falso, e tacere lascerebbe il
		// giocatore ad aspettare un click che non serve.
		View.Kind = ERTTargetPromptKind::SelfTargetConfirmed;
		View.Text = LOCTEXT("SelfTarget", "Azione su di se': confermata");
		return View;

	case ERTPointerTargetKind::Unit:
		View.Kind = ERTTargetPromptKind::ChooseUnit;
		View.Text = LOCTEXT("ChooseUnit", "Scegli un'unita'");
		View.bIsAwaitingTarget = true;
		return View;

	case ERTPointerTargetKind::Cell:
		View.Kind = ERTTargetPromptKind::ChooseCell;
		View.Text = LOCTEXT("ChooseCell", "Scegli una cella");
		View.bIsAwaitingTarget = true;
		return View;

	case ERTPointerTargetKind::Edge:
		View.Kind = ERTTargetPromptKind::ChooseEdge;
		View.Text = LOCTEXT("ChooseEdge", "Scegli un bordo");
		View.bIsAwaitingTarget = true;
		return View;

	case ERTPointerTargetKind::Object:
		break; // cade sotto: nessun produttore, e si dichiara invece di indovinare.
	}

	// ⚠️ **`Object` e' nell'enum e nessuno lo produce**: `TargetKindForAction` restituisce `None`, `Edge`,
	// `Cell` o `Unit`, mai `Object`. Non gli si inventa una frase — «scegli un oggetto» sarebbe una
	// promessa che nessun percorso mantiene — e non lo si lascia sparire in un testo vuoto, che e' il
	// difetto che `ComposePlayerEventText` documenta un piano piu' in la': *«una riga vuota e' una
	// sparizione che nessuno nota»*. Il giorno in cui un owner produrra' `Object`, questo ramo e il suo
	// test dicono dove aggiungere il prompt.
	View.Kind = ERTTargetPromptKind::Unsupported;
	View.Text = LOCTEXT("Unsupported", "Bersaglio non ancora supportato");
	View.bIsAwaitingTarget = true;
	return View;
}

#undef LOCTEXT_NAMESPACE

ERTPlanChip URTHudViewModel::ChipForWarning(const FRTUnitSlotsView& Slots, const FRTPlanWarningView& Warning)
{
	if (!Slots.bAuthorized || Warning.SourceActionId.IsNone())
	{
		return ERTPlanChip::None;
	}
	if (Slots.Main.bOccupied && Warning.SourceActionId == Slots.Main.ActionId)
	{
		return ERTPlanChip::Main;
	}
	if (Slots.Reaction.bOccupied && Warning.SourceActionId == Slots.Reaction.ActionId)
	{
		return ERTPlanChip::Reaction;
	}
	return Slots.Movement.bOccupied ? ERTPlanChip::Movement : ERTPlanChip::None;
}

ERTActionPhaseMark URTHudViewModel::SlotPhaseMark(const ARTUnit* Unit, const FRTPlannedSlotView& Slot)
{
	if (!Unit || !Slot.bOccupied)
	{
		return ERTActionPhaseMark::None;
	}
	if (Slot.ActionId.IsNone())
	{
		return ERTActionPhaseMark::Move; // un percorso: lo slot e' occupato senza un'azione
	}
	for (int32 i = 0; i < Unit->NumAbilities(); ++i)
	{
		const URTActionData* Azione = Unit->GetAbility(i);
		if (Azione && Azione->Def.ActionId == Slot.ActionId)
		{
			return PhaseMarkFor(Azione->Def);
		}
	}
	return ERTActionPhaseMark::None;
}
