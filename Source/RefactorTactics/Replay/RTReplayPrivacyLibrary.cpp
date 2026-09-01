#include "Replay/RTReplayPrivacyLibrary.h"

/**
 * La classificazione, campo per campo.
 *
 * 🔴 **`GET_MEMBER_NAME_CHECKED` e non un letterale**, e la differenza si vede su una rinomina: un
 * `TEXT("Priority")` sopravviverebbe alla rinomina del campo e continuerebbe a classificare un nome che
 * non esiste piu' — cioe' un campo reale tornerebbe **non classificato** mentre la tabella sembra piena.
 * Con la macro, la rinomina non compila. Il test `EveryLoggedFieldIsClassified` copre il caso residuo:
 * un campo aggiunto e mai nominato qui.
 *
 * ### Il criterio: COSA e' successo contro COME si e' deciso
 *
 * Pubblico e' il fatto risolto — chi, dove, quanto, in che fase. Di audit e' l'evidenza della decisione:
 * la conoscenza su cui si e' deciso e gli interni della risoluzione delle reazioni. E' il taglio che
 * [D-276](../../../docs/decisions/RT_PDR_00_Decision_Log.md) descrive quando assegna alla traccia privata
 * *«intenti privati, `TeamKnowledge` e altre evidenze side-private»*.
 *
 * ⚠️ **Nel dubbio, audit.** Tre campi stanno sul confine e sono qui in `AuditOnly` per default
 * fail-closed, non perche' la questione sia chiusa: `SelectedTargetUnitId` (il bersaglio effettivo di una
 * reazione — un fatto, ma che vive solo nelle voci di reazione), `OriginalTargetUnitId` (il bersaglio
 * prima della redirezione: e' **intento**, e per questo sta di qua) e `Priority` (l'ordine intra-fase, che
 * e' un interno del resolver e non qualcosa che si guarda). La ratifica e' una decisione d'autore; il gate
 * la rende visibile invece che implicita.
 */
const TMap<FName, ERTReplayFieldVisibility>& URTReplayPrivacyLibrary::FieldVisibility()
{
	static const TMap<FName, ERTReplayFieldVisibility> Tabella = {
		// --- Pubblico: cosa e' successo ---
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, Phase),                ERTReplayFieldVisibility::Public },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, Category),             ERTReplayFieldVisibility::Public },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, Outcome),              ERTReplayFieldVisibility::Public },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, SrcCell),              ERTReplayFieldVisibility::Public },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, TgtCell),              ERTReplayFieldVisibility::Public },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, Amount),               ERTReplayFieldVisibility::Public },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, ActionId),             ERTReplayFieldVisibility::Public },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, BaseActionId),         ERTReplayFieldVisibility::Public },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, UnitId),               ERTReplayFieldVisibility::Public },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, TurnNumber),           ERTReplayFieldVisibility::Public },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, GraphRevision),        ERTReplayFieldVisibility::Public },

		// --- Audit: come si e' deciso ---
		// Il verdetto di conoscenza: e' `Transient` per [D-223] e oggi non arriva nemmeno all'archivio.
		// Classificarlo comunque non e' ridondante — e' il campo che il lavoro di rendere durevole
		// l'evidenza dovra' spostare, e trovarlo gia' dichiarato audit e' la meta' di quella decisione.
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, Verdict),              ERTReplayFieldVisibility::AuditOnly },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, Priority),             ERTReplayFieldVisibility::AuditOnly },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, OpportunityId),        ERTReplayFieldVisibility::AuditOnly },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, ReactionInstanceId),   ERTReplayFieldVisibility::AuditOnly },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, SelectedTargetUnitId), ERTReplayFieldVisibility::AuditOnly },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, OriginalTargetUnitId), ERTReplayFieldVisibility::AuditOnly },
		{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, ReactionResponse),     ERTReplayFieldVisibility::AuditOnly },
	};
	return Tabella;
}

/**
 * L'unico ponte fra i due prodotti.
 *
 * ⚠️ **Non filtra le VOCI, filtra i CAMPI**, e la distinzione va detta invece che lasciata dedurre: il
 * filtro per osservatore — *«questa unita' la mia squadra l'aveva vista?»* — richiede la `TeamKnowledge`
 * del turno, che la traccia archiviata **non porta** (`FRTTurnLogEntry::Verdict` e' `Transient` per
 * [D-223]). Finche' quel dato non esiste in forma durevole, un ponte che accettasse un `ObserverTeamId`
 * prometterebbe un filtro che non puo' fare.
 *
 * ⚠️ **Copia in ordine e non riordina.** La traccia arriva gia' ordinata da `SortTurnLog`, e quell'ordine
 * *e'* il replay: riordinare qui produrrebbe una riproduzione diversa da cio' che la partita ha risolto.
 */
TArray<FRTPublicReplayEntry> URTReplayPrivacyLibrary::ToPublicTrace(const TArray<FRTTurnLogEntry>& AuditTrace)
{
	TArray<FRTPublicReplayEntry> Out;
	Out.Reserve(AuditTrace.Num());

	for (const FRTTurnLogEntry& E : AuditTrace)
	{
		FRTPublicReplayEntry P;
		P.Phase = E.Phase;
		P.Category = E.Category;
		P.Outcome = E.Outcome;
		P.SrcCell = E.SrcCell;
		P.TgtCell = E.TgtCell;
		P.Amount = E.Amount;
		P.ActionId = E.ActionId;
		P.BaseActionId = E.BaseActionId;
		P.UnitId = E.UnitId;
		P.TurnNumber = E.TurnNumber;
		P.GraphRevision = E.GraphRevision;
		Out.Add(P);
	}

	return Out;
}
