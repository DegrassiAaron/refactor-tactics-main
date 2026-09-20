#pragma once

// Il file di seduta: cosa e' stato giudicato, su quale albero, e con quale esito (#3208).

#include "CoreMinimal.h"
#include "PieSession/RTPieSessionTypes.h"

/**
 * Scrive `Saved/RTPieSessions/<sessionId>/session.json`.
 *
 * Un file per SEDUTA e non uno per scenario: la propagazione a valle deve poter leggere in un posto solo
 * cosa e' successo in una apertura, e da N cartelle `RTTests/` non si ricostruisce quali run
 * appartenessero alla stessa seduta.
 */
class REFACTORTACTICS_API URTPieSessionWriter
{
public:
	/**
	 * Il JSON della seduta.
	 *
	 * `Commit` vuoto diventa `"commit": null`, **non** un campo assente: un campo che manca si legge come
	 * «non pertinente», `null` si legge come «non lo so», e il secondo e' il fatto. In questo progetto una
	 * misura vale se avviene su un commit dichiarato, quindi non saperlo va detto, non nascosto.
	 */
	static FString ToJson(const FString& SessionId, const FString& Commit,
		const TArray<FRTPieSessionStep>& Steps);

	/** Scrive il file e restituisce il percorso in `OutPath`. */
	static bool Write(const FString& SessionId, const FString& Commit,
		const TArray<FRTPieSessionStep>& Steps, FString& OutPath, FString& OutError);

	/**
	 * Lo SHA di `HEAD` letto da `.git/HEAD`, vuoto se non si riesce.
	 *
	 * ⚠️ Dichiara il `HEAD` che legge, **non** lo stato del working tree: una seduta condotta su un albero
	 * sporco porta lo SHA del commit e nessuna promessa sul resto. Prometterla sarebbe peggio.
	 */
	static FString ReadHeadCommit();

	/** `YYYYMMDD-hhmmss` in UTC: ordina da solo e non collide fra due sedute nello stesso minuto. */
	static FString MakeSessionId(const FDateTime& Now);
};
