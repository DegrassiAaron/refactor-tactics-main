"""Regole di routing v1: dato un evento, chi lo riceve.

Modulo PURO. Nessun database, nessuna rete, nessun ambiente. E' scritto cosi' perche' le
regole sono la parte del control plane che si sbaglia piu' facilmente e che si prova
piu' a buon mercato: un test di routing non ha bisogno di un daemon acceso.

Una consegna e' indirizzata in uno di due modi, e solo uno dei due per volta:

    a una SESSIONE      `session_id` valorizzato   -> la prende quella e nessun'altra
    a un RUOLO su LANE  `role` + `lane`            -> la prende la prima sessione
                                                     compatibile che fa ack

Il secondo modo e' quello che rende il control plane utile: al momento in cui DEV
pubblica `TASK_READY`, l'EDITOR della sua lane puo' non esistere ancora. La consegna non
si perde e non si duplica - resta in attesa di un ruolo, non di un processo.

⛔ Le regole qui sotto NON decidono se il lavoro sia corretto. Instradano un messaggio.
La catena canonica DEV-LEAD -> EDITOR -> VALIDATION resta una regola di CONTRATTO
(RT3_CONTRACT.md), e nulla qui impedisce a una sessione di pubblicare un evento fuori
sequenza: il control plane la registra e la mostra, non la giudica.
"""

import collections

from .errors import InvalidEvent
from .model import LANES, ROLES, check_event_type

#: Un destinatario e' *o* una sessione *o* una coppia (ruolo, lane). Mai entrambi:
#: mescolarli produrrebbe una consegna che due sessioni credono propria.
Recipient = collections.namedtuple("Recipient", "session_id role lane")

#: Esito del routing. `rule` nomina la regola applicata e finisce nell'evento: senza,
#: un mancato recapito costringe a indovinare se la regola non esiste o non ha fatto
#: match.
RoutingResult = collections.namedtuple("RoutingResult", "recipients rule reason")

#: Gli eventi che ESIGONO un destinatario esplicito. Sono conversazione fra due
#: sessioni, e non esiste una regola sensata che indovini l'interlocutore: senza
#: destinatario, un `QUESTION` verrebbe registrato e non letto da nessuno, che e' il
#: modo peggiore di fallire.
DIRECTED_ONLY = ("QUESTION", "ANSWER")


def _to_recipient_role(role, lane):
    return Recipient(session_id=None, role=role, lane=lane)


def automatic_rule(event_type, sender_role, sender_lane):
    """La sola tabella delle regole automatiche. Ritorna (recipients, rule, reason).

    Ritorna una lista vuota quando nessuna regola fa match: e' un esito legittimo, non
    un errore. L'evento viene comunque registrato nell'event log.
    """
    # DEV che dichiara pronto il lavoro passa la mano all'EDITOR della propria lane.
    if sender_role == "DEV" and event_type == "TASK_READY":
        return (
            [_to_recipient_role("EDITOR", sender_lane)],
            "DEV_TASK_READY_TO_EDITOR_SAME_LANE",
            "TASK_READY da DEV va all'EDITOR della lane {}.".format(sender_lane),
        )

    # EDITOR che chiede validazione resta nella propria lane. La spec enumera DEV e
    # DESIGNER; la regola e' scritta come "stessa lane" e copre percio' anche MAIN, dove
    # a essere validata e' l'integrazione. Enumerare due lane su tre avrebbe lasciato
    # MAIN senza regola senza che nessuno lo avesse deciso.
    if sender_role == "EDITOR" and event_type == "VALIDATION_REQUESTED":
        return (
            [_to_recipient_role("VALIDATION", sender_lane)],
            "EDITOR_VALIDATION_REQUESTED_TO_VALIDATION_SAME_LANE",
            "VALIDATION_REQUESTED da EDITOR va a VALIDATION della lane {}.".format(
                sender_lane
            ),
        )

    # Il verdetto torna a chi lo ha chiesto.
    if sender_role == "VALIDATION" and event_type in (
        "VALIDATION_PASSED",
        "VALIDATION_FAILED",
    ):
        return (
            [_to_recipient_role("EDITOR", sender_lane)],
            "VALIDATION_VERDICT_TO_EDITOR_SAME_LANE",
            "{} da VALIDATION torna all'EDITOR della lane {}.".format(
                event_type, sender_lane
            ),
        )

    # L'integrazione converge sulla lane MAIN. ⚠️ Un EDITOR che sta GIA' su MAIN non e'
    # coperto: si consegnerebbe il messaggio da solo, che non e' un handoff ma un
    # promemoria. Resta senza regola apposta, ed e' visibile nel `reason`.
    if sender_role == "EDITOR" and event_type == "INTEGRATION_REQUESTED":
        if sender_lane == "MAIN":
            return (
                [],
                None,
                "INTEGRATION_REQUESTED da EDITOR gia' sulla lane MAIN non ha "
                "destinatario automatico: l'integrazione converge su MAIN, e MAIN e' "
                "il mittente. Usare --to-session o --to-role per indirizzarlo.",
            )
        return (
            [_to_recipient_role("EDITOR", "MAIN")],
            "EDITOR_INTEGRATION_REQUESTED_TO_EDITOR_MAIN",
            "INTEGRATION_REQUESTED da EDITOR lane {} va all'EDITOR della lane "
            "MAIN.".format(sender_lane),
        )

    return (
        [],
        None,
        "Nessuna regola automatica per {} da {} sulla lane {}.".format(
            event_type, sender_role, sender_lane
        ),
    )


def route(
    event_type,
    sender_role,
    sender_lane,
    recipient_session_id=None,
    recipient_role=None,
    recipient_lane=None,
):
    """Calcola i destinatari di un evento.

    Precedenza, dalla piu' forte alla piu' debole:

        1. `recipient_session_id`  - destinatario nominato, vince su tutto
        2. `recipient_role`        - ruolo nominato (lane esplicita, o quella del
                                     mittente se omessa)
        3. regola automatica       - la tabella di `automatic_rule`
        4. nessun destinatario     - evento registrato, non consegnato

    Un destinatario esplicito SOVRASCRIVE la regola automatica. E' voluto: la regola
    copre il caso normale, e l'operatore che nomina un destinatario sta dicendo che
    questo caso non e' normale.
    """
    check_event_type(event_type)
    if sender_role not in ROLES:
        raise InvalidEvent(
            "senderRole non valido: {!r}. Ammessi: {}.".format(
                sender_role, ", ".join(ROLES)
            )
        )
    if sender_lane not in LANES:
        raise InvalidEvent(
            "senderLane non valido: {!r}. Ammessi: {}.".format(
                sender_lane, ", ".join(LANES)
            )
        )

    if recipient_session_id and recipient_role:
        raise InvalidEvent(
            "destinatario ambiguo: sono stati indicati sia una sessione "
            "({}) sia un ruolo ({}). Indicarne uno solo.".format(
                recipient_session_id, recipient_role
            )
        )

    if recipient_session_id:
        return RoutingResult(
            [Recipient(session_id=recipient_session_id, role=None, lane=None)],
            "EXPLICIT_SESSION",
            "Destinatario nominato: sessione {}.".format(recipient_session_id),
        )

    if recipient_role:
        if recipient_role not in ROLES:
            raise InvalidEvent(
                "recipientRole non valido: {!r}. Ammessi: {}.".format(
                    recipient_role, ", ".join(ROLES)
                )
            )
        lane = recipient_lane or sender_lane
        if lane not in LANES:
            raise InvalidEvent(
                "recipientLane non valido: {!r}. Ammessi: {}.".format(
                    lane, ", ".join(LANES)
                )
            )
        return RoutingResult(
            [_to_recipient_role(recipient_role, lane)],
            "EXPLICIT_ROLE",
            "Destinatario nominato: ruolo {} sulla lane {}.".format(recipient_role, lane),
        )

    if event_type in DIRECTED_ONLY:
        raise InvalidEvent(
            "{} richiede un destinatario esplicito: non esiste una regola che "
            "indovini l'interlocutore di una conversazione. Usare --to-session "
            "<sessionId> oppure --to-role <ruolo> [--to-lane <lane>].".format(event_type)
        )

    recipients, rule, reason = automatic_rule(event_type, sender_role, sender_lane)
    return RoutingResult(recipients, rule, reason)
