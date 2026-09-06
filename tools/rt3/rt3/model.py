"""Vocabolario del control plane: ruoli, lane, tipi di evento, stati.

Quattro confini che RT3 tiene distinti, e che questo modulo NON collassa mai uno
sull'altro (la regola sta in CLAUDE.md e in `rt-task-router.ps1`):

    ruolo sessione     DEV | EDITOR | VALIDATION        cosa fa questa sessione
    workspace group    MAIN | DEV | DESIGNER            quale checkout ospita il terminale
    lane               MAIN | DEV | DESIGNER            su quale corsia viaggia il lavoro
    task               chi deve lavorare adesso         vive nel Task Registry

`WorkspaceGroup` e `Lane` hanno gli stessi tre valori e NON sono la stessa cosa: una
sessione EDITOR aperta nel checkout MAIN puo' lavorare sulla lane DEV, ed e' esattamente
lo scenario che l'integrazione produce. Sono due colonne separate apposta.
"""

import datetime
import re
import uuid

from .errors import InvalidEvent

# ---------------------------------------------------------------------------
# Enumerazioni
# ---------------------------------------------------------------------------

ROLES = ("DEV", "EDITOR", "VALIDATION")
WORKSPACE_GROUPS = ("MAIN", "DEV", "DESIGNER")
LANES = ("MAIN", "DEV", "DESIGNER")
WRITE_MODES = ("READ_ONLY", "WRITER")
UNREAL_LEASE_STATES = ("NONE", "REQUESTED", "OWNED")
SESSION_STATUSES = ("ACTIVE", "STOPPED")

#: Stato di una singola consegna. `PENDING` significa "nessuno l'ha ancora presa";
#: `ACKED` porta con se' CHI l'ha presa, perche' senza quel dato l'ack non risponde alla
#: domanda che conta - se il lavoro e' stato raccolto dalla sessione giusta.
DELIVERY_STATES = ("PENDING", "ACKED")

TASK_STATUSES = ("ACTIVE", "BLOCKED", "DONE")

#: I tipi di evento che il control plane sa validare, salvare, instradare e mostrare.
#: NON tutti hanno una regola di routing automatico (vedi `routing.py`): un tipo senza
#: regola resta pubblicabile, e viaggia con un destinatario esplicito.
EVENT_TYPES = (
    "SESSION_STARTED",
    "SESSION_STOPPED",
    "TASK_STARTED",
    "TASK_BLOCKED",
    "TASK_READY",
    "QUESTION",
    "ANSWER",
    "REVIEW_REQUESTED",
    "REVIEW_APPROVED",
    "REVIEW_REJECTED",
    "CANDIDATE_CREATED",
    "VALIDATION_REQUESTED",
    "VALIDATION_PASSED",
    "VALIDATION_FAILED",
    "INTEGRATION_REQUESTED",
    "INTEGRATION_ACCEPTED",
    "INTEGRATION_REJECTED",
    "LEASE_REQUESTED",
    "LEASE_GRANTED",
    "LEASE_RELEASED",
    "UNREAL_LEASE_REQUESTED",
    "UNREAL_LEASE_GRANTED",
    "UNREAL_LEASE_RELEASED",
)

#: Un SessionId e' scritto a mano dall'operatore e finisce in messaggi, log e nomi di
#: file. Restringerlo a questo alfabeto evita il caso in cui un id con uno spazio o una
#: barra rende illeggibile un `rt3 sessions list` - e, piu' avanti, impedisce che un id
#: diventi un path.
SESSION_ID_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$")

#: Il TaskId segue la stessa forma dei task RT3 esistenti (numeri di issue GitHub, ma
#: non solo: le wave usano id come `2272` o `rt3-control-plane`).
TASK_ID_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$")


# ---------------------------------------------------------------------------
# Helper
# ---------------------------------------------------------------------------


def now_iso():
    """Timestamp UTC ISO-8601 con suffisso `Z`.

    UTC e non ora locale: i confronti fra eventi devono restare validi anche
    attraversando l'ora legale, e un `LastSeenAt` che va all'indietro di un'ora sembra
    una sessione morta.
    """
    return (
        datetime.datetime.now(datetime.timezone.utc)
        .replace(microsecond=0)
        .isoformat()
        .replace("+00:00", "Z")
    )


def new_event_id():
    """Id evento stabile e leggibile a voce: `ev_` + 12 esadecimali."""
    return "ev_" + uuid.uuid4().hex[:12]


def new_delivery_id():
    return "dl_" + uuid.uuid4().hex[:12]


def new_candidate_id():
    return "cand_" + uuid.uuid4().hex[:12]


def _check_enum(value, allowed, field):
    if value not in allowed:
        raise InvalidEvent(
            "{} non valido: {!r}. Ammessi: {}.".format(field, value, ", ".join(allowed))
        )
    return value


def check_role(value):
    return _check_enum(value, ROLES, "role")


def check_lane(value):
    return _check_enum(value, LANES, "lane")


def check_workspace_group(value):
    return _check_enum(value, WORKSPACE_GROUPS, "workspaceGroup")


def check_write_mode(value):
    return _check_enum(value, WRITE_MODES, "writeMode")


def check_unreal_lease(value):
    return _check_enum(value, UNREAL_LEASE_STATES, "unrealLease")


def check_event_type(value):
    return _check_enum(value, EVENT_TYPES, "type")


def check_task_status(value):
    return _check_enum(value, TASK_STATUSES, "taskStatus")


def check_session_id(value):
    if not isinstance(value, str) or not SESSION_ID_RE.match(value):
        raise InvalidEvent(
            "sessionId non valido: {!r}. Ammessi lettere, cifre, punto, trattino e "
            "underscore, da 1 a 64 caratteri, primo carattere alfanumerico.".format(value)
        )
    return value


def check_task_id(value):
    if value is None:
        return None
    if not isinstance(value, str) or not TASK_ID_RE.match(value):
        raise InvalidEvent(
            "taskId non valido: {!r}. Stessa forma del sessionId.".format(value)
        )
    return value
