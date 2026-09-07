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


# -- Resource enforcement ----------------------------------------------------
#
# Gli exit code ripartono da 30: 20-29 appartengono alla roadmap. Un rifiuto di lease
# non e' un errore di forma - e' la risposta CORRETTA a una richiesta legittima che
# arriva seconda - e chi lo riceve deve poterlo distinguere da un guasto.


class ResourceAlreadyOwned(Rt3Error):
    """Una risorsa esclusiva e' gia' di qualcun altro.

    ⛔ Porta SEMPRE il proprietario, la risorsa e chi ha chiesto. Un «gia' occupato»
    senza il nome di chi la tiene lascia l'operatore a cercarlo a mano, ed e' il momento
    in cui serve di piu': due terminali aperti, e non si sa quale fermare.
    """

    code = "RT3_RESOURCE_ALREADY_OWNED"
    exit_code = 30

    def __init__(
        self,
        message,
        resource_type=None,
        resource_key=None,
        owner=None,
        requester=None,
        code=None,
        exit_code=None,
    ):
        # ⚠️ `code` e `exit_code` servono a `client.py`, che ricostruisce l'errore dal
        # codice ricevuto passandoli come keyword. Un `__init__` che non li accettasse
        # trasformerebbe un rifiuto legittimo in un TypeError dentro il client - cioe'
        # nasconderebbe la risposta corretta dietro un guasto apparente.
        super().__init__(message, code=code, exit_code=exit_code)
        self.resource_type = resource_type
        self.resource_key = resource_key
        self.owner_session_id = owner
        self.requester_session_id = requester

    def as_dict(self):
        return {
            "code": self.code,
            "resourceType": self.resource_type,
            "resourceKey": self.resource_key,
            "ownerSessionId": self.owner_session_id,
            "requesterSessionId": self.requester_session_id,
            "message": self.message,
        }


class WriterAlreadyOwned(ResourceAlreadyOwned):
    """`WriterCount(WorktreePath) <= 1` e' stato difeso: questa e' la seconda richiesta."""

    code = "RT3_WRITER_ALREADY_OWNED"
    exit_code = 30


class UnrealAlreadyOwned(ResourceAlreadyOwned):
    code = "RT3_UNREAL_ALREADY_OWNED"
    exit_code = 31


class TerminalAlreadyOpen(Rt3Error):
    """Una sessione ha gia' un terminale gestito vivo.

    Non e' un guasto: e' il vincolo che impedisce a due finestre di dichiararsi
    entrambe «il terminale» della stessa sessione, e quindi a RT3 di non sapere quale
    chiudere.
    """

    code = "RT3_TERMINAL_ALREADY_OPEN"
    exit_code = 31


class TerminalNotFound(Rt3Error):
    code = "RT3_TERMINAL_NOT_FOUND"
    exit_code = 32


class TerminalSpawnFailed(Rt3Error):
    """La finestra non si e' aperta. Chi chiama DEVE annullare cio' che aveva gia'
    fatto: sessione registrata e lease presi."""

    code = "RT3_TERMINAL_SPAWN_FAILED"
    exit_code = 33


class LeaseNotFound(Rt3Error):
    code = "RT3_LEASE_NOT_FOUND"
    exit_code = 32


class NotLeaseOwner(Rt3Error):
    """Rilasciare il lease di un altro richiede `--force`, e resta tracciato."""

    code = "RT3_NOT_LEASE_OWNER"
    exit_code = 33


class UnknownSessionField(Rt3Error):
    """Un campo che `update_session` non conosce e' un ERRORE, non un no-op.

    🔴 Ignorarlo in silenzio produce il peggior tipo di guasto: il comando riesce, il
    campo resta com'era, e chi legge crede di aver cambiato qualcosa. Misurato durante
    l'audit del 2026-09-06 con `unrealLease` scritto al posto di `unreal_lease`: nessun
    errore, nessun effetto, e una conclusione sbagliata tratta da una lista vuota.
    """

    code = "RT3_UNKNOWN_SESSION_FIELD"
    exit_code = 34
