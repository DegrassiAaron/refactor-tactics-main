#include "Replay/RTReplayPrivacyLibrary.h"

#include "UObject/UnrealType.h"

namespace
{
	struct FRTFieldVisibilityRow
	{
		FName Field;
		ERTReplayFieldVisibility Visibility;
	};

	/**
	 * Le righe della classificazione, in un array e non direttamente in una `TMap`.
	 *
	 * 🔴 **Una `TMap` costruita da initializer list INGOIA una chiave duplicata**, e l'ultima riga vince in
	 * silenzio: lo stesso campo elencato in tutti e due i blocchi passerebbe ogni gate — `Contains` e' vero,
	 * non ci sono fantasmi, la mappa e' coerente — mentre chi legge il file lo vede classificato nel blocco
	 * sbagliato. Con l'array la duplicazione e' misurabile, ed e' misurata subito sotto.
	 */
	const TArray<FRTFieldVisibilityRow>& VisibilityRows()
	{
		static const TArray<FRTFieldVisibilityRow> Rows = {
			// --- Pubblico: COSA e' successo ------------------------------------------------------------
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

			// ⚠️ I tre che sembravano di audit, e non lo sono. Il criterio non e' «nel dubbio si nasconde»:
			// e' se nasconderli tolga davvero qualcosa a chi legge il prodotto pubblico.
			//
			//  · `Priority` — il suo stesso docstring lo dichiara «una FUNZIONE di `ActionId`», che qui e'
			//    pubblico. Chi ha l'azione ricava la priorita' dal catalogo: nasconderla non protegge niente
			//    e toglie l'ordine di risoluzione a chi guarda.
			//  · `OriginalTargetUnitId` — «il trasferimento e' GIA' discriminato da `SrcCell`, che nell'hash
			//    c'e'; questo campo rende quel fatto LEGGIBILE senza inferenza, non lo aggiunge». Nasconderlo
			//    lascerebbe il fatto deducibile dalla cella e obbligherebbe alla deduzione che [D-063] ha
			//    dichiarato non valida.
			//  · `SelectedTargetUnitId` — l'argomento e' opposto e porta alla stessa conclusione: il suo
			//    docstring dice che dedurre l'unita' dalla `TgtCell` NON e' valido, perche' fra il micro-step
			//    e la fine del turno quella cella puo' cambiare occupante. E' un fatto risolto — a chi e'
			//    arrivato il colpo — e senza di lui il prodotto pubblico non sa nominare il bersaglio.
			{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, Priority),             ERTReplayFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, SelectedTargetUnitId), ERTReplayFieldVisibility::Public },
			{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, OriginalTargetUnitId), ERTReplayFieldVisibility::Public },

			// --- Audit: COME si e' deciso --------------------------------------------------------------
			//
			//  · `Verdict` — la conoscenza. E' `Transient` per la nota sul campo stesso (non per [D-223], che
			//    ne decide il CONGELAMENTO alla scrittura e non la serializzazione) e oggi non arriva nemmeno
			//    all'archivio: classificarlo comunque non e' ridondante, perche' e' il campo che dovra'
			//    spostarsi il giorno in cui l'evidenza di audit diventera' durevole, e trovarlo gia'
			//    dichiarato e' meta' di quella decisione.
			//  · `OpportunityId` — l'identita' della finestra di reazione. Su una risposta `HOLD` dice che
			//    quell'unita' AVEVA una reazione armata e ha scelto di non usarla: e' capacita' e decisione,
			//    non un fatto che si sia visto accadere.
			//  · `ReactionInstanceId` — QUALE delle reazioni armate ha risposto. Stessa famiglia: rende la
			//    traccia spiegabile, e cio' che spiega e' l'armamento di chi decide.
			//  · `ReactionResponse` — la risposta di profilo. Il suo docstring dichiara che due risposte
			//    DIVERSE con lo stesso esito danno lo stesso hash: non e' deducibile da `Outcome`, quindi
			//    metterla nel prodotto pubblico aggiungerebbe davvero la decisione a cio' che si vede.
			{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, Verdict),              ERTReplayFieldVisibility::AuditOnly },
			// · `VerdictSubject` — contro CHI quel verdetto e' stato congelato. Sta di qua per la stessa
			//   ragione del verdetto, e il gate ha preteso che lo dichiarassi: aggiungerlo senza classificarlo
			//   avrebbe reso rosso `EveryLoggedFieldIsClassified`, che e' esattamente il suo mestiere.
			{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, VerdictSubject),       ERTReplayFieldVisibility::AuditOnly },
			{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, OpportunityId),        ERTReplayFieldVisibility::AuditOnly },
			{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, ReactionInstanceId),   ERTReplayFieldVisibility::AuditOnly },
			{ GET_MEMBER_NAME_CHECKED(FRTTurnLogEntry, ReactionResponse),     ERTReplayFieldVisibility::AuditOnly },
		};
		return Rows;
	}

	/** La coppia di proprieta' riflesse di un campo pubblico, risolta una volta sola. */
	struct FRTPublicFieldBinding
	{
		const FProperty* Source = nullptr;
		const FProperty* Target = nullptr;
	};

	/**
	 * Il legame fra i due tipi, costruito **dalla tabella**.
	 *
	 * 🔴 Una copia scritta a mano campo per campo sarebbe una SECONDA lista, e la seconda lista e' il
	 * difetto: chi classificasse un campo nuovo come pubblico, lo aggiungesse al tipo pubblico e si
	 * dimenticasse la riga di assegnamento avrebbe tutti e quattro i test verdi e ogni voce del replay con
	 * quel campo a zero. Una classificazione che mente non e' un leak, ma e' lo stesso tipo di silenzio.
	 */
	const TArray<FRTPublicFieldBinding>& PublicFieldBindings()
	{
		static const TArray<FRTPublicFieldBinding> Bindings = []
		{
			TArray<FRTPublicFieldBinding> Built;
			for (const FRTFieldVisibilityRow& Row : VisibilityRows())
			{
				if (Row.Visibility != ERTReplayFieldVisibility::Public)
				{
					continue;
				}

				FRTPublicFieldBinding Binding;
				Binding.Source = FRTTurnLogEntry::StaticStruct()->FindPropertyByName(Row.Field);
				Binding.Target = FRTPublicReplayEntry::StaticStruct()->FindPropertyByName(Row.Field);

				// Non un `check`: un crash in partita e' peggio di un campo mancante, e il gate
				// `PublicEntryMatchesTheClassification` rende questa condizione irraggiungibile.
				if (!ensureAlwaysMsgf(Binding.Source && Binding.Target && Binding.Source->SameType(Binding.Target),
					TEXT("Il campo pubblico '%s' non ha una controparte dello stesso tipo in FRTPublicReplayEntry"),
					*Row.Field.ToString()))
				{
					continue;
				}

				Built.Add(Binding);
			}
			return Built;
		}();
		return Bindings;
	}
}

const TMap<FName, ERTReplayFieldVisibility>& URTReplayPrivacyLibrary::FieldVisibility()
{
	static const TMap<FName, ERTReplayFieldVisibility> Table = []
	{
		TMap<FName, ERTReplayFieldVisibility> Built;
		for (const FRTFieldVisibilityRow& Row : VisibilityRows())
		{
			ensureAlwaysMsgf(!Built.Contains(Row.Field),
				TEXT("Il campo '%s' e' classificato due volte: la seconda riga vincerebbe in silenzio"),
				*Row.Field.ToString());
			Built.Add(Row.Field, Row.Visibility);
		}
		return Built;
	}();
	return Table;
}

/**
 * Il ponte.
 *
 * ⚠️ **Copia in ordine e non riordina.** La traccia arriva gia' ordinata da `SortTurnLog`, e quell'ordine
 * *e'* il replay: riordinare qui produrrebbe una riproduzione diversa da cio' che la partita ha risolto.
 * Il legame fra i campi si risolve una volta sola, non per voce.
 */
TArray<FRTPublicReplayEntry> URTReplayPrivacyLibrary::ToPublicTrace(const TArray<FRTTurnLogEntry>& AuditTrace)
{
	const TArray<FRTPublicFieldBinding>& Bindings = PublicFieldBindings();

	TArray<FRTPublicReplayEntry> Out;
	Out.Reserve(AuditTrace.Num());

	for (const FRTTurnLogEntry& Entry : AuditTrace)
	{
		FRTPublicReplayEntry Public;
		for (const FRTPublicFieldBinding& Binding : Bindings)
		{
			Binding.Target->CopyCompleteValue(
				Binding.Target->ContainerPtrToValuePtr<void>(&Public),
				Binding.Source->ContainerPtrToValuePtr<void>(&Entry));
		}
		Out.Add(MoveTemp(Public));
	}

	return Out;
}
