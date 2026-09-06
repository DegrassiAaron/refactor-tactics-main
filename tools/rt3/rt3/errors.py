"""Errori RT3.

Ogni errore porta un CODICE stabile, non solo un messaggio. Il codice e' cio' che una
sessione puo' confrontare senza leggere la prosa, ed e' la stessa scelta gia' fatta da
`rt-task-router.ps1` (TASK_NOT_FOUND, TASK_ROUTE_MISMATCH...) e da `rt-workspace.ps1`.

La CLI stampa `CODICE: messaggio` e ritorna un exit code non zero. Non stampa mai un
traceback come comportamento normale: un traceback e' la prova che un caso non e' stato
previsto, e va distinto da un rifiuto previsto.
"""


class Rt3Error(Exception):
    """Errore previsto: la CLI lo stampa come `CODICE: messaggio` senza traceback."""

    code = "RT3_ERROR"
    exit_code = 1

    def __init__(self, message, code=None, exit_code=None):
        super().__init__(message)
        self.message = message
        if code is not None:
            self.code = code
        if exit_code is not None:
            self.exit_code = exit_code

    def __str__(self):
        return "{}: {}".format(self.code, self.message)


class DaemonUnavailable(Rt3Error):
    """`rt3d` non risponde. Non e' un bug: e' lo stato normale prima di avviarlo."""

    code = "RT3_DAEMON_UNAVAILABLE"
    exit_code = 3


class ProtocolMismatch(Rt3Error):
    """Client e daemon parlano protocolli diversi: fail-fast, non degradazione.

    Degradare qui sarebbe peggio del rifiuto, perche' il sintomo comparirebbe piu'
    tardi e altrove - su un evento consegnato a meta', o su un campo letto come None.
    """

    code = "RT3_PROTOCOL_MISMATCH"
    exit_code = 4


class SchemaMismatch(Rt3Error):
    """Il database ha uno schema che questo binario non sa gestire."""

    code = "RT3_SCHEMA_MISMATCH"
    exit_code = 4


class StoreUnavailable(Rt3Error):
    """La radice dello store non e' localizzabile o il database non e' apribile."""

    code = "RT3_STORE_UNAVAILABLE"
    exit_code = 5


class SessionUnbound(Rt3Error):
    """Il terminale corrente non e' legato ad alcuna sessione RT3."""

    code = "RT3_SESSION_UNBOUND"
    exit_code = 6


class SessionExists(Rt3Error):
    """SessionId gia' registrato e ancora vivo."""

    code = "RT3_SESSION_EXISTS"
    exit_code = 7


class SessionNotFound(Rt3Error):
    code = "RT3_SESSION_NOT_FOUND"
    exit_code = 8


class InvalidEvent(Rt3Error):
    code = "RT3_INVALID_EVENT"
    exit_code = 9


class EventNotFound(Rt3Error):
    code = "RT3_EVENT_NOT_FOUND"
    exit_code = 10


class DeliveryConflict(Rt3Error):
    """L'ack e' arrivato dopo quello di un'altra sessione.

    Non e' un errore del chiamante: e' la corsa che il modello a mailbox ammette, e
    l'unica risposta onesta e' dire chi ha vinto.
    """

    code = "RT3_DELIVERY_CONFLICT"
    exit_code = 11


class TaskNotFound(Rt3Error):
    code = "RT3_TASK_NOT_FOUND"
    exit_code = 12


class NotAuthorized(Rt3Error):
    """La sessione non e' destinataria della delivery che sta tentando di consumare."""

    code = "RT3_NOT_AUTHORIZED"
    exit_code = 13


# -- Roadmap Orchestration ---------------------------------------------------
#
# Gli exit code ripartono da 20 e non da 14: i codici sopra appartengono al control
# plane, questi alla roadmap. Lo stacco lascia spazio al primo gruppo senza dover
# rinumerare il secondo - e un exit code che cambia significato fra due versioni e' un
# guasto silenzioso in ogni script che lo confronti.


class RoadmapNotFound(Rt3Error):
    """Nessuna roadmap caricata con quell'id. Non e' un errore di forma del file."""

    code = "RT3_ROADMAP_NOT_FOUND"
    exit_code = 21


class RoadmapAmbiguous(Rt3Error):
    """Piu' roadmap caricate e nessun `--id`: scegliere e' del chiamante, non nostro."""

    code = "RT3_ROADMAP_AMBIGUOUS"
    exit_code = 22


class ItemNotFound(Rt3Error):
    """La issue non esiste nella roadmap caricata."""

    code = "RT3_ITEM_NOT_FOUND"
    exit_code = 23


class CandidateNotFound(Rt3Error):
    code = "RT3_CANDIDATE_NOT_FOUND"
    exit_code = 24
