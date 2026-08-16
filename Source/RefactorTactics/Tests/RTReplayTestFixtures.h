#pragma once

#include "CoreMinimal.h"
#include "Replay/RTReplayManifest.h"
#include "Replay/RTReplayRecorderLibrary.h"
#include "Replay/RTMatchHistoryLibrary.h"
#include "Turn/RTTurnLog.h"
#include "Turn/RTTurnLogLibrary.h"
#include "Turn/RTTurnRules.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Fixture condivisa dai test del replay.
 *
 * 🔴 Nasce il 2026-08-16 da una duplicazione trovata in code review: gli helper di
 * `RTReplayViewerSubsystemTests.cpp` erano quelli di `RTReplayViewModelTests.cpp` con un suffisso
 * cambiato, e uno dei due — il costruttore di tracce — era pure **meno capace** dell'originale, con le
 * fasi cablate invece che parametriche. Due copie da correggere due volte, e la seconda gia' peggiorata
 * mentre la si scriveva.
 *
 * ⚠️ **Copre i due file del replay e non il terzo.** `RTFrontendNavigationTests.cpp` contiene la stessa
 * costruzione di `UGameInstance` (`MakeNavigator`/`ReleaseNavigator`), e non e' unificata qui perche' quel
 * file appartiene alla track `frontend_shell` (`#936`): un path non assegnato e' uno **stop**, non un
 * invito (`D-139`). `MakeSubsystemHost` sotto e' scritto per accoglierlo il giorno in cui quella track lo
 * porti qui — e' un template, non una funzione legata al replay.
 */
namespace RTReplayFixtures
{
	/**
	 * Una radice isolata sotto la cartella transiente dell'automation.
	 *
	 * ⚠️ Si chiama `TransientRoot` e non `Root` di proposito: i test dichiarano quasi sempre una variabile
	 * locale per la propria radice, e una funzione chiamata `Root` importata con `using namespace` le
	 * andrebbe addosso.
	 */
	inline FString TransientRoot(const TCHAR* Nome)
	{
		return FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("RTReplay"), Nome);
	}

	inline void Pulisci(const FString& InRoot)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*InRoot)) { PF.DeleteDirectoryRecursively(*InRoot); }
	}

	inline FRTTurnLogEntry Voce(int32 TurnoDichiarato, ERTMatchPhase Fase, int32 Amount)
	{
		FRTTurnLogEntry E;
		E.TurnNumber = TurnoDichiarato;
		E.Phase = Fase;
		E.Category = ERTLogCategory::Move;
		E.ActionId = FName(TEXT("Action.Move"));
		E.Amount = Amount;
		E.SrcCell = FRTCellId(Amount, 0);
		E.TgtCell = FRTCellId(Amount + 1, 0);
		return E;
	}

	/**
	 * Una traccia che dichiara `TurnoDichiarato` e contiene una voce per ciascuna fase richiesta.
	 *
	 * ⚠️ Il turno **dichiarato dalle voci** e' un parametro separato dal numero di file, e non e' un
	 * artificio: `RTReplaySeekLibrary` documenta che una sequenza puo' iniziare da un turno qualsiasi, e
	 * `OpenArchive` carica i file per posizione. Sono due cose diverse, e un test che le confonde
	 * verificherebbe un archivio che il codice non promette.
	 */
	inline TArray<FRTTurnLogEntry> Traccia(int32 TurnoDichiarato, const TArray<ERTMatchPhase>& Fasi)
	{
		TArray<FRTTurnLogEntry> Voci;
		int32 N = 0;
		for (const ERTMatchPhase F : Fasi)
		{
			Voci.Add(Voce(TurnoDichiarato, F, ++N));
		}
		URTTurnLogLibrary::SortTurnLog(Voci); // forma canonica: precondizione del seek e del Player
		return Voci;
	}

	/** Scrive un archivio dalle tracce date, nell'ordine. `bChiudi = false` lo lascia parziale. */
	inline FGuid ScriviArchivio(const FString& InRoot, const TArray<TArray<FRTTurnLogEntry>>& Tracce,
		bool bChiudi, bool bIndicizza = false, const FDateTime& Inizio = FDateTime(2026, 8, 16, 9, 0, 0))
	{
		FRTReplayManifest M;
		M.MatchId = FGuid::NewGuid();
		M.FormatId = FName(TEXT("Format.Skirmish2v2"));
		M.bHexTopology = true;

		for (int32 i = 0; i < Tracce.Num(); ++i)
		{
			URTReplayRecorderLibrary::RecordTurn(InRoot, M, i + 1, Tracce[i]);
		}
		if (bChiudi)
		{
			URTReplayRecorderLibrary::CloseMatch(InRoot, M, ERTMatchOutcome::Team0Wins, 4242, 12.5f);
		}
		if (bIndicizza)
		{
			// L'indice e' un file separato, e non lo scrive il recorder: lo scrive chi registra la partita.
			URTMatchHistoryLibrary::AppendOrUpdate(InRoot,
				URTMatchHistoryLibrary::EntryFromManifest(M, Inizio));
		}
		return M.MatchId;
	}

	/** Due turni pieni con due fasi ciascuno: l'archivio di base della maggior parte dei test. */
	inline FGuid ArchivioDueTurni(const FString& InRoot, bool bChiudi = true, bool bIndicizza = false)
	{
		TArray<TArray<FRTTurnLogEntry>> Tracce;
		Tracce.Add(Traccia(1, { ERTMatchPhase::Blast, ERTMatchPhase::Move }));
		Tracce.Add(Traccia(2, { ERTMatchPhase::Blast, ERTMatchPhase::Move }));
		return ScriviArchivio(InRoot, Tracce, bChiudi, bIndicizza);
	}

	/**
	 * Una `UGameInstance` viva e il subsystem richiesto. Serve un host — non un mondo, non un viewport.
	 *
	 * Template e non funzione: la costruzione non ha nulla di specifico del replay, ed e' identica a
	 * quella che i test del frontend fanno per il navigatore.
	 */
	template <typename T>
	T* MakeSubsystemHost(UGameInstance*& OutGI)
	{
		OutGI = NewObject<UGameInstance>(GetTransientPackage());
		if (!OutGI)
		{
			return nullptr;
		}
		OutGI->AddToRoot();
		OutGI->Init();
		return OutGI->GetSubsystem<T>();
	}

	inline void ReleaseSubsystemHost(UGameInstance* GI)
	{
		if (GI)
		{
			GI->Shutdown();
			GI->RemoveFromRoot();
		}
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
