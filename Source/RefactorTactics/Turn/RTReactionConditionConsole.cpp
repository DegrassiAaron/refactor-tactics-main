#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Player/RTPlayerController.h"
#include "Turn/RTReactionOpportunityTypes.h"
#include "Unit/RTUnit.h"

/**
 * `rt.Reaction.Condition [percentuale]` — dichiara la condizione sulla reazione armata dell'unita' selezionata:
 * «rispondi solo se il bersaglio e' a N% di salute o meno» ([D-109]). Senza argomenti, toglie la condizione.
 *
 * **Perche' esiste un comando e non solo l'interfaccia.** La condizione dichiarata e' il mitigatore di pacing
 * del regime `Conditional`: senza, ogni trigger di Overwatch aprira' 3 secondi di attesa appena CP 14.5 cabla
 * la finestra. L'interfaccia di pianificazione che la esporra' e' lavoro di E11, e finche' non esiste il campo
 * resterebbe scritto **solo dai test** — la stessa asimmetria che ha tenuto `ReactionPlanning` fuori dalle
 * capability dell'harness fino a `#601`, e che rende un verde bugiardo: il gioco direbbe che il giocatore puo'
 * dichiarare qualcosa che non ha modo di chiedere.
 *
 * Non e' un `rt.Debug.*`: quel namespace e' per gli strumenti di sola lettura (CP 11.4). Questo **cambia il
 * piano**, quindi sta nel proprio, e la validazione e' la stessa che usera' l'interfaccia — `SetPlannedReactionCondition`,
 * non una scorciatoia che scrive il campo.
 */
static void RTReactionConditionCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (!World)
	{
		Ar.Logf(ELogVerbosity::Warning, TEXT("[RT] Nessun mondo: comando ignorato."));
		return;
	}

	ARTPlayerController* PC = Cast<ARTPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
	if (!PC)
	{
		Ar.Logf(ELogVerbosity::Warning, TEXT("[RT] Nessun controller di gioco: il comando vive in partita."));
		return;
	}

	ARTUnit* Unit = PC->GetSelectedUnit();
	if (!Unit)
	{
		Ar.Logf(ELogVerbosity::Warning, TEXT("[RT] Nessuna unita' selezionata: seleziona chi deve porre la condizione."));
		return;
	}

	// Senza argomenti si TOGLIE la condizione: e' il modo di tornare a «rispondi comunque», ed e' sempre
	// legittimo — anche quando non c'e' una reazione armata.
	if (Args.Num() == 0)
	{
		Unit->SetPlannedReactionCondition(FRTDeclaredCondition());
		Ar.Logf(TEXT("[RT] %s: condizione rimossa (risponde a ogni trigger)."), *Unit->GetName());
		return;
	}

	// Percentuale INTERA: `FCString::Atoi` su un argomento non numerico da' 0, che sarebbe una soglia valida e
	// silenziosamente diversa da quella scritta. Meglio rifiutare cio' che non e' un numero.
	const FString& Raw = Args[0];
	if (!Raw.IsNumeric())
	{
		Ar.Logf(ELogVerbosity::Warning,
			TEXT("[RT] «%s» non e' una percentuale intera. Uso: rt.Reaction.Condition 50"), *Raw);
		return;
	}

	const FRTDeclaredCondition Condition(
		URTReactionOpportunityLibrary::TargetHealthAtOrBelowPercent(), FCString::Atoi(*Raw));

	if (!Unit->SetPlannedReactionCondition(Condition))
	{
		// Il rifiuto e' DETTO, come per una reazione in ricarica: una condizione che sparisse in silenzio
		// lascerebbe il giocatore convinto di aver ristretto il fuoco, e la scoperta arriverebbe a turno risolto.
		if (Unit->PlannedReactionAbility == INDEX_NONE)
		{
			Ar.Logf(ELogVerbosity::Warning,
				TEXT("[RT] %s non ha una reazione armata: arma prima la reazione, poi ponile la condizione."),
				*Unit->GetName());
		}
		else
		{
			Ar.Logf(ELogVerbosity::Warning,
				TEXT("[RT] Condizione rifiutata: la soglia dev'essere una percentuale fra 0 e 100."));
		}
		return;
	}

	Ar.Logf(TEXT("[RT] %s: risponde solo se il bersaglio e' al %d%% di salute o meno."),
		*Unit->GetName(), Condition.Param);
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTReactionCondition(
	TEXT("rt.Reaction.Condition"),
	TEXT("Dichiara la condizione sulla reazione armata dell'unita' selezionata: «rispondi solo se il bersaglio "
		 "e' a N% di salute o meno». Senza argomenti toglie la condizione. Cambia il piano, non e' diagnostica."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTReactionConditionCommand));
