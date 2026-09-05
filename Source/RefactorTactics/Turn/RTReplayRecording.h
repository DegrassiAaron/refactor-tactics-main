#pragma once

#include "CoreMinimal.h"
#include "Replay/RTReplayManifest.h" // FRTReplayManifest
#include "Turn/RTTurnLog.h"          // FRTTurnLogEntry
#include "Core/RTTypes.h"            // ERTMatchOutcome
#include "RTReplayRecording.generated.h"

struct FRTTurnAudit;

/**
 * Lo **stato** di un archivio replay in scrittura, e il suo ciclo di vita (`#2286`, nona fetta di `#1818`).
 *
 * 🔑 **Perche' esce da `ARTTurnManager`.** Era il terzo — e ultimo — carico *inerte all'esito* che quella
 * classe teneva: registra cio' che il resolver ha gia' deciso, e nessuna regola lo rilegge. La matematica
 * era gia' fuori, in tre librerie pure; qui restavano il manifest, due timestamp e la sequenza.
 *
 * ⛔ **Non logga, e non e' un dettaglio.** Tutti e tre i metodi che ha sostituito chiamavano `AddLogEvent`
 * per segnalare un fallimento — cioe' scrivevano nel TurnLog del manager. Un registratore che loggasse da
 * se' dovrebbe conoscere il TurnLog e il TurnManager: esattamente la dipendenza che questa estrazione
 * esiste per togliere. Qui si **restituisce un esito**, e chi chiama decide se e come dirlo.
 *
 * ⚠️ **Non raccoglie i fatti, li riceve.** Le squadre osservatrici, l'osservatore locale e l'audit del turno
 * si compongono interrogando il mondo e lo stato della partita, e sono cose che l'orchestratore possiede.
 * Portarle qui avrebbe trascinato dentro `UGameplayStatics` e quattro buffer del manager — la stessa
 * ragione per cui `CollectPacingUnitFacts` e' rimasta di la' nella fetta precedente.
 */
USTRUCT()
struct REFACTORTACTICS_API FRTReplayRecording
{
	GENERATED_BODY()

	/**
	 * La radice in cui scrivere: l'override se c'e', altrimenti il default della libreria.
	 *
	 * ⚠️ `Override` arriva come parametro perche' e' una `UPROPERTY` **editabile da Blueprint** sul
	 * TurnManager: spostarla qui romperebbe gli asset che la impostano.
	 */
	static FString ResolveRoot(const FString& Override);

	/**
	 * Apre un archivio nuovo. Chi chiama ha gia' deciso **se** registrare e ha gia' raccolto gli osservatori.
	 *
	 * ⛔ La guardia su `bRecordReplay` e sul formato resta **fuori**: sono due condizioni che il TurnManager
	 * conosce e questa struct no, e duplicarle qui creerebbe due posti in cui la stessa domanda ha risposta.
	 */
	void Begin(FName FormatId, TArray<int32> ObserverTeamIds, int32 LocalObserverTeamId, const FString& Root);

	/** `true` fra un `Begin` riuscito e la chiusura: c'e' un archivio a cui scrivere. */
	bool IsRecording() const { return Manifest.MatchId.IsValid(); }

	/** `true` se l'archivio e' gia' stato chiuso: riscriverlo sarebbe un secondo finale. */
	bool IsClosed() const { return Manifest.bClosed; }

	/** L'identita' della partita registrata; invalida finche' `Begin` non e' passato. */
	FGuid GetMatchId() const { return Manifest.MatchId; }

	/** Le voci del turno finiscono nell'archivio. `false` = non registrato, e chi chiama lo dice. */
	bool RecordTurn(const FString& Root, int32 TurnNumber, const TArray<FRTTurnLogEntry>& TurnLog);

	/** L'audit del turno, gia' composto da chi possiede i fatti. `false` = non registrato. */
	bool RecordAudit(const FString& Root, const FRTTurnAudit& Audit) const;

	/**
	 * L'ultimo hash ordinato scritto, `0` se nessuno: e' cio' che l'audit del turno cita.
	 *
	 * ⚠️ Si legge **dopo** `RecordTurn`, non prima: e' quella chiamata a scriverlo.
	 */
	uint32 LastOrderedHash() const;

	/** Chiude l'archivio col suo esito. `false` = non chiuso, e resta aperto. */
	bool Close(const FString& Root, ERTMatchOutcome Outcome, uint32 FinalStateHash);

	/** Quando la registrazione e' cominciata, in UTC: lo storico partite lo cita. */
	const FDateTime& GetStartedUtc() const { return StartedUtc; }

	/** Il manifest, per chi deve leggerlo interamente. */
	const FRTReplayManifest& GetManifest() const { return Manifest; }

private:
	UPROPERTY()
	FRTReplayManifest Manifest;

	/** Origine reale per la durata a orologio: la scrive `Begin`, la legge `Close`. */
	double StartRealSeconds = 0.0;

	UPROPERTY()
	FDateTime StartedUtc;
};
