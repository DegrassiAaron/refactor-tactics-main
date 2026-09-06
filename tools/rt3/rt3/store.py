"""Persistenza SQLite del control plane.

Un solo processo scrive questo database: `rt3d`. La CLI non lo apre mai - parla HTTP col
daemon. E' la topologia imposta dalla specifica (§3: tutte le sessioni comunicano con
`rt3d`, nessuna comunicazione terminale -> terminale), e ha un effetto collaterale
utile: il file non viene mai toccato da due processi che non si conoscono.

Le transazioni servono comunque, perche' il daemon e' multi-thread: tre terminali che
fanno `inbox ack` sullo stesso istante sono tre thread sullo stesso database. WAL piu'
`BEGIN IMMEDIATE` sulle mutazioni bastano, e sono cio' che la specifica chiede al posto
di un lock su file scritto a mano.

⚠️ L'ack e' la sola operazione che ha una corsa vera, ed e' risolta dal database e non
dal codice applicativo:

    UPDATE deliveries SET state='ACKED' ... WHERE delivery_id=? AND state='PENDING'

Se `rowcount` e' zero, un'altra sessione e' arrivata prima. Nessun read-modify-write,
nessuna finestra fra il controllo e la scrittura.
"""

import json
import os
import sqlite3
import threading

from . import SCHEMA_VERSION
from .errors import (
    DeliveryConflict,
    EventNotFound,
    NotAuthorized,
    SchemaMismatch,
    SessionExists,
    SessionNotFound,
    StoreUnavailable,
    TaskNotFound,
)
from .model import (
    DELIVERY_STATES,
    check_event_type,
    check_lane,
    check_role,
    check_session_id,
    check_task_id,
    check_task_status,
    check_unreal_lease,
    check_workspace_group,
    check_write_mode,
    new_candidate_id,
    new_delivery_id,
    new_event_id,
    now_iso,
)
from .routing import route

# ---------------------------------------------------------------------------
# Schema
# ---------------------------------------------------------------------------

#: Migrazioni indicizzate dalla versione che PRODUCONO. `migrate()` applica in ordine
#: quelle mancanti. Aggiungere una versione qui e alzare SCHEMA_VERSION in __init__.py:
#: sono due gesti, e separarli e' l'errore che lascia un database non migrato.
_MIGRATIONS = {}


def _migration(version):
    def wrap(fn):
        _MIGRATIONS[version] = fn
        return fn

    return wrap


def _split_statements(script):
    """Divide un DDL in statement, ignorando i `;` nelle righe di commento.

    ⛔ Uno `script.split(";")` non basta, e il modo in cui si scopre e' istruttivo: il
    commento sopra la tabella `candidates` conteneva la frase «...che vive in Git; un
    lease e' una annotazione...». Il punto e virgola della PROSA spezzava lo statement a
    meta', e SQLite riportava `near "un": syntax error` - un errore che nomina una
    parola italiana e non dice nulla dello statement vero.

    ⚠️ Copre le righe di SOLO commento, che sono la forma usata qui. Un `;` in un
    commento messo in coda a del codice (`col TEXT, -- nota; altro`) romperebbe ancora:
    non scriverne.
    """
    statements, buffer = [], []
    for line in script.splitlines():
        if line.strip().startswith("--"):
            buffer.append(line)
            continue
        while ";" in line:
            head, line = line.split(";", 1)
            buffer.append(head)
            statements.append("\n".join(buffer))
            buffer = []
        buffer.append(line)
    tail = "\n".join(buffer)
    if tail.strip():
        statements.append(tail)
    return [s for s in statements if s.strip()]


def _exec_ddl(conn, script):
    """Esegue un DDL statement per statement.

    ⚠️ NON usare `conn.executescript`: fa un COMMIT implicito prima di partire, quindi
    chiude la transazione che `migrate()` ha appena aperto e fa fallire il COMMIT
    successivo con "cannot commit - no transaction is active". Il sintomo si presenta
    come un database non apribile, e la causa e' una riga altrove.
    """
    for statement in _split_statements(script):
        conn.execute(statement)


@_migration(1)
def _v1(conn):
    _exec_ddl(
        conn,
        """
        CREATE TABLE sessions (
            session_id       TEXT PRIMARY KEY,
            role             TEXT NOT NULL,
            workspace_group  TEXT NOT NULL,
            lane             TEXT NOT NULL,
            worktree_path    TEXT,
            repo_root        TEXT,
            -- NULL legittimo: detached HEAD. Non usare stringa vuota, che si confonde
            -- con un branch senza nome e rompe i confronti.
            branch           TEXT,
            head             TEXT,
            task_id          TEXT,
            candidate_id     TEXT,
            write_set        TEXT NOT NULL DEFAULT '[]',
            write_mode       TEXT NOT NULL DEFAULT 'READ_ONLY',
            unreal_lease     TEXT NOT NULL DEFAULT 'NONE',
            started_at       TEXT NOT NULL,
            last_seen_at     TEXT NOT NULL,
            stopped_at       TEXT,
            status           TEXT NOT NULL DEFAULT 'ACTIVE',
            host             TEXT,
            client_pid       INTEGER
        );
        CREATE INDEX idx_sessions_role_lane ON sessions(role, lane, status);

        CREATE TABLE tasks (
            task_id     TEXT PRIMARY KEY,
            title       TEXT,
            status      TEXT NOT NULL DEFAULT 'ACTIVE',
            lane        TEXT,
            created_at  TEXT NOT NULL,
            updated_at  TEXT NOT NULL
        );

        -- `seq` e non il timestamp e' l'ordine autorevole degli eventi: due eventi
        -- pubblicati nello stesso secondo hanno lo stesso `created_at`, e un ordine
        -- ambiguo in un event log e' un difetto che si manifesta solo sotto carico.
        CREATE TABLE events (
            seq                INTEGER PRIMARY KEY AUTOINCREMENT,
            event_id           TEXT NOT NULL UNIQUE,
            type               TEXT NOT NULL,
            created_at         TEXT NOT NULL,
            sender_session_id  TEXT NOT NULL,
            sender_role        TEXT NOT NULL,
            sender_lane        TEXT NOT NULL,
            task_id            TEXT,
            candidate_id       TEXT,
            payload            TEXT NOT NULL DEFAULT '{}',
            note               TEXT,
            routing_rule       TEXT,
            routing_reason     TEXT
        );
        CREATE INDEX idx_events_task ON events(task_id);
        CREATE INDEX idx_events_type ON events(type);

        -- Una consegna e' indirizzata *o* a una sessione *o* a (ruolo, lane). Il CHECK
        -- rende impossibile il terzo caso - entrambi valorizzati - che produrrebbe una
        -- consegna che due sessioni credono propria.
        CREATE TABLE deliveries (
            delivery_id           TEXT PRIMARY KEY,
            event_id              TEXT NOT NULL REFERENCES events(event_id),
            recipient_session_id  TEXT,
            recipient_role        TEXT,
            recipient_lane        TEXT,
            state                 TEXT NOT NULL DEFAULT 'PENDING',
            created_at            TEXT NOT NULL,
            acked_at              TEXT,
            acked_by              TEXT,
            CHECK (
                (recipient_session_id IS NOT NULL AND recipient_role IS NULL)
                OR
                (recipient_session_id IS NULL AND recipient_role IS NOT NULL)
            )
        );
        CREATE INDEX idx_deliveries_pending
            ON deliveries(state, recipient_session_id, recipient_role, recipient_lane);

        -- Candidati e lease: il control plane ne tiene i METADATI, non la cosa. Un
        -- candidate e' un riferimento a branch/sha che vive in Git; un lease e' una
        -- annotazione su chi dice di occupare una risorsa. L'enforcement e' della
        -- milestone successiva, e il modello dati e' pronto a riceverlo.
        CREATE TABLE candidates (
            candidate_id  TEXT PRIMARY KEY,
            task_id       TEXT,
            session_id    TEXT,
            branch        TEXT,
            head          TEXT,
            note          TEXT,
            created_at    TEXT NOT NULL
        );

        CREATE TABLE leases (
            lease_id     TEXT PRIMARY KEY,
            resource     TEXT NOT NULL,
            holder       TEXT,
            state        TEXT NOT NULL,
            acquired_at  TEXT,
            released_at  TEXT,
            note         TEXT
        );

        CREATE TABLE meta (
            key    TEXT PRIMARY KEY,
            value  TEXT NOT NULL
        );
        """
    )


# ---------------------------------------------------------------------------
# Store
# ---------------------------------------------------------------------------


def _row_to_dict(row):
    return dict(row) if row is not None else None


def _session_to_json(row):
    d = _row_to_dict(row)
    if d is None:
        return None
    try:
        d["write_set"] = json.loads(d.get("write_set") or "[]")
    except ValueError:
        d["write_set"] = []
    return d


class Store:
    """Accesso al database. Una connessione per thread, WAL, foreign key attive."""

    def __init__(self, path):
        self.path = path
        self._local = threading.local()
        self._init_lock = threading.Lock()

    # -- connessione ------------------------------------------------------

    def connect(self):
        conn = getattr(self._local, "conn", None)
        if conn is not None:
            return conn
        directory = os.path.dirname(os.path.abspath(self.path))
        if directory:
            try:
                os.makedirs(directory, exist_ok=True)
            except OSError as exc:
                raise StoreUnavailable(
                    "impossibile creare la directory dello store {}: {}".format(
                        directory, exc
                    )
                )
        try:
            conn = sqlite3.connect(self.path, timeout=15.0, isolation_level=None)
            conn.row_factory = sqlite3.Row
            conn.execute("PRAGMA journal_mode=WAL")
            conn.execute("PRAGMA foreign_keys=ON")
            conn.execute("PRAGMA busy_timeout=15000")
        except sqlite3.Error as exc:
            raise StoreUnavailable(
                "database RT3 non apribile ({}): {}. Se il file e' corrotto, fermare "
                "rt3d, spostarlo e riavviare: il control plane si ricrea vuoto.".format(
                    self.path, exc
                )
            )
        self._local.conn = conn
        return conn

    def close(self):
        conn = getattr(self._local, "conn", None)
        if conn is not None:
            conn.close()
            self._local.conn = None

    # -- migrazioni -------------------------------------------------------

    def schema_version(self):
        conn = self.connect()
        try:
            row = conn.execute(
                "SELECT value FROM meta WHERE key='schema_version'"
            ).fetchone()
        except sqlite3.OperationalError:
            return 0  # tabella meta assente: database vergine
        if row is None:
            return 0
        try:
            return int(row["value"])
        except (TypeError, ValueError):
            return 0

    def migrate(self):
        """Porta il database a SCHEMA_VERSION. Fail-fast se e' piu' nuovo del codice."""
        with self._init_lock:
            conn = self.connect()
            current = self.schema_version()
            if current > SCHEMA_VERSION:
                raise SchemaMismatch(
                    "il database {} ha schema v{}, questo rt3 conosce fino a v{}. "
                    "Il control plane di questo workspace e' piu' VECCHIO di quello "
                    "che ha scritto il database: aggiornare il workspace invece di "
                    "degradare il database.".format(self.path, current, SCHEMA_VERSION)
                )
            if current == SCHEMA_VERSION:
                return current
            for version in range(current + 1, SCHEMA_VERSION + 1):
                migration = _MIGRATIONS.get(version)
                if migration is None:
                    raise SchemaMismatch(
                        "manca la migrazione per lo schema v{}.".format(version)
                    )
                conn.execute("BEGIN IMMEDIATE")
                try:
                    migration(conn)
                    conn.execute(
                        "INSERT INTO meta(key, value) VALUES('schema_version', ?) "
                        "ON CONFLICT(key) DO UPDATE SET value=excluded.value",
                        (str(version),),
                    )
                    conn.execute("COMMIT")
                except Exception:
                    conn.execute("ROLLBACK")
                    raise
            return SCHEMA_VERSION

    # -- sessioni ---------------------------------------------------------

    def start_session(
        self,
        session_id,
        role,
        workspace_group,
        lane,
        worktree_path=None,
        repo_root=None,
        branch=None,
        head=None,
        task_id=None,
        candidate_id=None,
        write_set=None,
        write_mode="READ_ONLY",
        unreal_lease="NONE",
        host=None,
        client_pid=None,
        replace=False,
    ):
        """Registra una sessione.

        Un `session_id` gia' ATTIVO e' rifiutato: due terminali che credono di essere
        `DEV-1` producono consegne che finiscono nel posto sbagliato, ed e' meglio un
        rifiuto immediato di un errore che si manifesta tre eventi dopo. Un id di una
        sessione FERMATA viene riusato senza discutere - e' il caso normale di un
        terminale riaperto - e `replace=True` forza il riuso anche di una attiva, che
        e' la via d'uscita per una sessione morta senza cleanup.
        """
        check_session_id(session_id)
        check_role(role)
        check_workspace_group(workspace_group)
        check_lane(lane)
        check_write_mode(write_mode)
        check_unreal_lease(unreal_lease)
        check_task_id(task_id)

        conn = self.connect()
        ts = now_iso()
        payload_write_set = json.dumps(list(write_set or []))

        conn.execute("BEGIN IMMEDIATE")
        try:
            row = conn.execute(
                "SELECT status FROM sessions WHERE session_id=?", (session_id,)
            ).fetchone()
            if row is not None and row["status"] == "ACTIVE" and not replace:
                raise SessionExists(
                    "la sessione {} risulta gia' ATTIVA. Se il terminale precedente e' "
                    "morto senza fermarla, ripubblicarla con --replace oppure "
                    "`rt3 session stop --id {}`.".format(session_id, session_id)
                )
            conn.execute(
                """
                INSERT INTO sessions(
                    session_id, role, workspace_group, lane, worktree_path, repo_root,
                    branch, head, task_id, candidate_id, write_set, write_mode,
                    unreal_lease, started_at, last_seen_at, stopped_at, status,
                    host, client_pid)
                VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,NULL,'ACTIVE',?,?)
                ON CONFLICT(session_id) DO UPDATE SET
                    role=excluded.role,
                    workspace_group=excluded.workspace_group,
                    lane=excluded.lane,
                    worktree_path=excluded.worktree_path,
                    repo_root=excluded.repo_root,
                    branch=excluded.branch,
                    head=excluded.head,
                    task_id=excluded.task_id,
                    candidate_id=excluded.candidate_id,
                    write_set=excluded.write_set,
                    write_mode=excluded.write_mode,
                    unreal_lease=excluded.unreal_lease,
                    started_at=excluded.started_at,
                    last_seen_at=excluded.last_seen_at,
                    stopped_at=NULL,
                    status='ACTIVE',
                    host=excluded.host,
                    client_pid=excluded.client_pid
                """,
                (
                    session_id,
                    role,
                    workspace_group,
                    lane,
                    worktree_path,
                    repo_root,
                    branch,
                    head,
                    task_id,
                    candidate_id,
                    payload_write_set,
                    write_mode,
                    unreal_lease,
                    ts,
                    ts,
                    host,
                    client_pid,
                ),
            )
            if task_id:
                self._upsert_task_locked(conn, task_id, lane=lane)
            conn.execute("COMMIT")
        except Exception:
            conn.execute("ROLLBACK")
            raise
        return self.get_session(session_id)

    def get_session(self, session_id, required=False):
        conn = self.connect()
        row = conn.execute(
            "SELECT * FROM sessions WHERE session_id=?", (session_id,)
        ).fetchone()
        if row is None and required:
            raise SessionNotFound(
                "sessione {} non registrata su questa macchina.".format(session_id)
            )
        return _session_to_json(row)

    def list_sessions(self, include_stopped=False):
        conn = self.connect()
        if include_stopped:
            rows = conn.execute(
                "SELECT * FROM sessions ORDER BY lane, role, session_id"
            ).fetchall()
        else:
            rows = conn.execute(
                "SELECT * FROM sessions WHERE status='ACTIVE' "
                "ORDER BY lane, role, session_id"
            ).fetchall()
        return [_session_to_json(r) for r in rows]

    def touch_session(self, session_id):
        """Aggiorna `last_seen_at`. Non fallisce se la sessione non esiste piu'."""
        conn = self.connect()
        conn.execute(
            "UPDATE sessions SET last_seen_at=? WHERE session_id=?",
            (now_iso(), session_id),
        )

    def update_session(self, session_id, **fields):
        """Aggiorna i campi mutabili di una sessione registrata."""
        # I campi NON elencati qui sono immutabili per costruzione: role, lane e
        # workspace_group descrivono che cosa la sessione e', e cambiarli sotto le
        # consegne gia' instradate le farebbe atterrare in una mailbox diversa da
        # quella per cui erano state calcolate. Si cambiano fermando la sessione e
        # registrandone un'altra.
        validators = {
            "task_id": check_task_id,
            "candidate_id": None,
            "branch": None,
            "head": None,
            "write_mode": check_write_mode,
            "unreal_lease": check_unreal_lease,
            "write_set": None,
        }
        self.get_session(session_id, required=True)
        sets, values = [], []
        for key, value in fields.items():
            if key not in validators or value is None:
                continue
            validator = validators[key]
            if validator is not None:
                validator(value)
            if key == "write_set":
                value = json.dumps(list(value))
            sets.append("{}=?".format(key))
            values.append(value)
        if not sets:
            return self.get_session(session_id)
        sets.append("last_seen_at=?")
        values.append(now_iso())
        values.append(session_id)
        conn = self.connect()
        conn.execute(
            "UPDATE sessions SET {} WHERE session_id=?".format(", ".join(sets)),
            tuple(values),
        )
        return self.get_session(session_id)

    def stop_session(self, session_id):
        session = self.get_session(session_id, required=True)
        conn = self.connect()
        ts = now_iso()
        conn.execute(
            "UPDATE sessions SET status='STOPPED', stopped_at=?, last_seen_at=? "
            "WHERE session_id=?",
            (ts, ts, session_id),
        )
        return self.get_session(session_id)

    # -- task -------------------------------------------------------------

    def _upsert_task_locked(self, conn, task_id, title=None, status=None, lane=None):
        ts = now_iso()
        conn.execute(
            """
            INSERT INTO tasks(task_id, title, status, lane, created_at, updated_at)
            VALUES(?,?,COALESCE(?,'ACTIVE'),?,?,?)
            ON CONFLICT(task_id) DO UPDATE SET
                title=COALESCE(excluded.title, tasks.title),
                status=COALESCE(?, tasks.status),
                lane=COALESCE(excluded.lane, tasks.lane),
                updated_at=excluded.updated_at
            """,
            (task_id, title, status, lane, ts, ts, status),
        )

    def upsert_task(self, task_id, title=None, status=None, lane=None):
        check_task_id(task_id)
        if status is not None:
            check_task_status(status)
        if lane is not None:
            check_lane(lane)
        conn = self.connect()
        conn.execute("BEGIN IMMEDIATE")
        try:
            self._upsert_task_locked(conn, task_id, title, status, lane)
            conn.execute("COMMIT")
        except Exception:
            conn.execute("ROLLBACK")
            raise
        return self.get_task(task_id)

    def get_task(self, task_id, required=False):
        conn = self.connect()
        row = conn.execute("SELECT * FROM tasks WHERE task_id=?", (task_id,)).fetchone()
        if row is None:
            if required:
                raise TaskNotFound(
                    "task {} sconosciuto al control plane: nessuna sessione e nessun "
                    "evento lo ha ancora nominato.".format(task_id)
                )
            return None
        task = _row_to_dict(row)
        task["sessions"] = [
            _session_to_json(r)
            for r in conn.execute(
                "SELECT * FROM sessions WHERE task_id=? ORDER BY lane, role", (task_id,)
            ).fetchall()
        ]
        task["events"] = [
            _row_to_dict(r)
            for r in conn.execute(
                "SELECT * FROM events WHERE task_id=? ORDER BY seq", (task_id,)
            ).fetchall()
        ]
        return task

    def list_tasks(self):
        conn = self.connect()
        return [
            _row_to_dict(r)
            for r in conn.execute("SELECT * FROM tasks ORDER BY updated_at DESC")
        ]

    # -- eventi e consegne ------------------------------------------------

    def publish(
        self,
        sender_session_id,
        event_type,
        task_id=None,
        candidate_id=None,
        payload=None,
        note=None,
        recipient_session_id=None,
        recipient_role=None,
        recipient_lane=None,
    ):
        """Pubblica un evento e crea le consegne che il routing produce.

        Evento e consegne nascono nella STESSA transazione. Separarle lascerebbe la
        finestra in cui un evento e' registrato ma non consegnato: invisibile in
        `inbox`, presente nell'event log, cioe' il difetto piu' difficile da diagnosticare
        di tutto il control plane.
        """
        check_event_type(event_type)
        sender = self.get_session(sender_session_id, required=True)
        check_task_id(task_id)
        if recipient_session_id:
            check_session_id(recipient_session_id)

        result = route(
            event_type,
            sender["role"],
            sender["lane"],
            recipient_session_id=recipient_session_id,
            recipient_role=recipient_role,
            recipient_lane=recipient_lane,
        )

        # Un destinatario nominato deve esistere come SessionId noto, altrimenti la
        # consegna resterebbe pending per sempre in attesa di un id scritto male. Un
        # destinatario per RUOLO non ha questo vincolo: e' il caso in cui la sessione
        # non e' ancora nata, e la mailbox offline esiste apposta.
        for recipient in result.recipients:
            if recipient.session_id:
                self.get_session(recipient.session_id, required=True)

        # Un `task_id` sull'evento e' anche una dichiarazione che il task esiste.
        effective_task = task_id or sender.get("task_id")

        conn = self.connect()
        event_id = new_event_id()
        ts = now_iso()
        conn.execute("BEGIN IMMEDIATE")
        try:
            conn.execute(
                """
                INSERT INTO events(
                    event_id, type, created_at, sender_session_id, sender_role,
                    sender_lane, task_id, candidate_id, payload, note, routing_rule,
                    routing_reason)
                VALUES(?,?,?,?,?,?,?,?,?,?,?,?)
                """,
                (
                    event_id,
                    event_type,
                    ts,
                    sender_session_id,
                    sender["role"],
                    sender["lane"],
                    effective_task,
                    candidate_id,
                    json.dumps(payload or {}),
                    note,
                    result.rule,
                    result.reason,
                ),
            )
            delivery_ids = []
            for recipient in result.recipients:
                delivery_id = new_delivery_id()
                conn.execute(
                    """
                    INSERT INTO deliveries(
                        delivery_id, event_id, recipient_session_id, recipient_role,
                        recipient_lane, state, created_at)
                    VALUES(?,?,?,?,?,'PENDING',?)
                    """,
                    (
                        delivery_id,
                        event_id,
                        recipient.session_id,
                        recipient.role,
                        recipient.lane,
                        ts,
                    ),
                )
                delivery_ids.append(delivery_id)
            if effective_task:
                self._upsert_task_locked(conn, effective_task, lane=sender["lane"])
            conn.execute(
                "UPDATE sessions SET last_seen_at=? WHERE session_id=?",
                (ts, sender_session_id),
            )
            conn.execute("COMMIT")
        except Exception:
            conn.execute("ROLLBACK")
            raise

        return {
            "eventId": event_id,
            "type": event_type,
            "createdAt": ts,
            "routingRule": result.rule,
            "routingReason": result.reason,
            "deliveries": delivery_ids,
            "recipients": [r._asdict() for r in result.recipients],
        }

    def get_event(self, event_id, required=True):
        conn = self.connect()
        row = conn.execute(
            "SELECT * FROM events WHERE event_id=?", (event_id,)
        ).fetchone()
        if row is None:
            if required:
                raise EventNotFound("evento {} inesistente.".format(event_id))
            return None
        event = _row_to_dict(row)
        try:
            event["payload"] = json.loads(event.get("payload") or "{}")
        except ValueError:
            event["payload"] = {}
        event["deliveries"] = [
            _row_to_dict(r)
            for r in conn.execute(
                "SELECT * FROM deliveries WHERE event_id=? ORDER BY delivery_id",
                (event_id,),
            ).fetchall()
        ]
        return event

    def list_events(self, limit=50, task_id=None):
        conn = self.connect()
        if task_id:
            rows = conn.execute(
                "SELECT * FROM events WHERE task_id=? ORDER BY seq DESC LIMIT ?",
                (task_id, limit),
            ).fetchall()
        else:
            rows = conn.execute(
                "SELECT * FROM events ORDER BY seq DESC LIMIT ?", (limit,)
            ).fetchall()
        return [_row_to_dict(r) for r in rows]

    # -- mailbox ----------------------------------------------------------

    def _inbox_query(self, session):
        """Predicato di visibilita' di una consegna per una sessione.

        Una sessione vede una consegna se e' indirizzata a LEI per nome, oppure al suo
        RUOLO sulla sua LANE. Non vede nient'altro - ed e' cosi' che due sessioni
        incompatibili non consumano lo stesso messaggio: un EDITOR della lane DESIGNER
        non vede le consegne dirette a (EDITOR, DEV), quindi non puo' nemmeno provare a
        farne ack.

        Fra sessioni COMPATIBILI - due EDITOR sulla stessa lane - la consegna e' invece
        condivisa, e vince chi fa ack per primo. E' voluto: la consegna e' indirizzata a
        un ruolo, non a un processo, e un secondo EDITOR sulla stessa lane e' per
        definizione un sostituto legittimo.
        """
        return (
            "(d.recipient_session_id = ?"
            " OR (d.recipient_session_id IS NULL"
            "     AND d.recipient_role = ? AND d.recipient_lane = ?))",
            (session["session_id"], session["role"], session["lane"]),
        )

    def inbox(self, session_id, state="PENDING", limit=100):
        session = self.get_session(session_id, required=True)
        where, params = self._inbox_query(session)
        conn = self.connect()
        sql = (
            "SELECT d.*, e.type, e.created_at AS event_created_at, "
            "       e.sender_session_id, e.sender_role, e.sender_lane, "
            "       e.task_id, e.candidate_id, e.note, e.routing_rule "
            "FROM deliveries d JOIN events e ON e.event_id = d.event_id "
            "WHERE {} ".format(where)
        )
        args = list(params)
        if state and state != "ALL":
            if state not in DELIVERY_STATES:
                state = "PENDING"
            sql += "AND d.state = ? "
            args.append(state)
        sql += "ORDER BY e.seq ASC LIMIT ?"
        args.append(limit)
        rows = conn.execute(sql, tuple(args)).fetchall()
        self.touch_session(session_id)
        return [_row_to_dict(r) for r in rows]

    def pending_count(self, session_id):
        session = self.get_session(session_id, required=True)
        where, params = self._inbox_query(session)
        conn = self.connect()
        row = conn.execute(
            "SELECT COUNT(*) AS n FROM deliveries d WHERE {} AND d.state='PENDING'".format(
                where
            ),
            params,
        ).fetchone()
        return int(row["n"])

    def _resolve_delivery(self, conn, session, ref):
        """Trova la consegna visibile a `session` a partire da un delivery_id o event_id.

        Accettare l'event_id e' una comodita' che conta: chi legge `inbox list` vede il
        tipo e l'evento, e chiedere di ricopiare un secondo identificatore per fare ack
        e' esattamente il travaso manuale che il control plane esiste per togliere.
        """
        where, params = self._inbox_query(session)
        row = conn.execute(
            "SELECT d.* FROM deliveries d WHERE d.delivery_id=? AND {}".format(where),
            (ref,) + tuple(params),
        ).fetchone()
        if row is not None:
            return row
        row = conn.execute(
            "SELECT d.* FROM deliveries d JOIN events e ON e.event_id=d.event_id "
            "WHERE d.event_id=? AND {} ORDER BY d.state DESC LIMIT 1".format(where),
            (ref,) + tuple(params),
        ).fetchone()
        if row is not None:
            return row
        # Distinguere "non esiste" da "non e' tuo": sono due diagnosi diverse, e
        # confonderle manda a cercare un id sbagliato quando il problema e' il ruolo.
        exists = conn.execute(
            "SELECT 1 FROM deliveries WHERE delivery_id=? OR event_id=? LIMIT 1",
            (ref, ref),
        ).fetchone()
        if exists is not None:
            raise NotAuthorized(
                "{} esiste ma non e' indirizzato a {} ({} sulla lane {}).".format(
                    ref, session["session_id"], session["role"], session["lane"]
                )
            )
        raise EventNotFound("nessuna consegna o evento con id {}.".format(ref))

    def show(self, session_id, ref):
        session = self.get_session(session_id, required=True)
        conn = self.connect()
        delivery = _row_to_dict(self._resolve_delivery(conn, session, ref))
        event = self.get_event(delivery["event_id"])
        self.touch_session(session_id)
        return {"delivery": delivery, "event": event}

    def ack(self, session_id, ref, note=None):
        """Acknowledgement esclusivo e atomico.

        Il `WHERE ... AND state='PENDING'` e' il punto: due sessioni compatibili che
        fanno ack nello stesso istante producono un solo vincitore, e il perdente riceve
        RT3_DELIVERY_CONFLICT con il nome di chi ha preso il messaggio. Non c'e' nessun
        controllo-poi-scrivi in mezzo.
        """
        session = self.get_session(session_id, required=True)
        conn = self.connect()
        ts = now_iso()
        conn.execute("BEGIN IMMEDIATE")
        try:
            delivery = self._resolve_delivery(conn, session, ref)
            cur = conn.execute(
                "UPDATE deliveries SET state='ACKED', acked_at=?, acked_by=? "
                "WHERE delivery_id=? AND state='PENDING'",
                (ts, session_id, delivery["delivery_id"]),
            )
            won = cur.rowcount == 1
            row = conn.execute(
                "SELECT * FROM deliveries WHERE delivery_id=?",
                (delivery["delivery_id"],),
            ).fetchone()
            conn.execute(
                "UPDATE sessions SET last_seen_at=? WHERE session_id=?", (ts, session_id)
            )

            # Prendere in carico un evento significa prendere in carico il suo TASK.
            # Senza questa riga, l'EDITOR che raccoglie un TASK_READY del task 2272
            # dovrebbe poi ripassare `--task 2272` a mano su ogni evento che pubblica -
            # cioe' proprio il travaso manuale che il control plane esiste per togliere.
            # Lo smoke test lo ha scoperto cosi': il task mostrava i due eventi di DEV e
            # non i due della risposta.
            #
            # ⚠️ Adotta SOLO se la sessione non ha gia' un task proprio. Sovrascrivere un
            # task dichiarato esplicitamente sposterebbe una sessione su un lavoro che
            # non ha scelto, e sarebbe peggio del problema che risolve.
            if won:
                event_task = conn.execute(
                    "SELECT task_id FROM events WHERE event_id=?",
                    (delivery["event_id"],),
                ).fetchone()
                if event_task and event_task["task_id"] and not session.get("task_id"):
                    conn.execute(
                        "UPDATE sessions SET task_id=? WHERE session_id=? "
                        "AND task_id IS NULL",
                        (event_task["task_id"], session_id),
                    )
            conn.execute("COMMIT")
        except Exception:
            conn.execute("ROLLBACK")
            raise

        result = _row_to_dict(row)
        if not won:
            raise DeliveryConflict(
                "la consegna {} era gia' stata presa da {} alle {}.".format(
                    result["delivery_id"], result["acked_by"], result["acked_at"]
                )
            )
        if note:
            self.connect().execute(
                "UPDATE events SET note = COALESCE(note || ' | ', '') || ? "
                "WHERE event_id=?",
                (note, result["event_id"]),
            )
        return result

    # -- candidati e lease (metadati) -------------------------------------

    def create_candidate(
        self, session_id, task_id=None, branch=None, head=None, note=None
    ):
        session = self.get_session(session_id, required=True)
        candidate_id = new_candidate_id()
        conn = self.connect()
        conn.execute(
            "INSERT INTO candidates(candidate_id, task_id, session_id, branch, head, "
            "note, created_at) VALUES(?,?,?,?,?,?,?)",
            (
                candidate_id,
                task_id or session.get("task_id"),
                session_id,
                branch if branch is not None else session.get("branch"),
                head if head is not None else session.get("head"),
                note,
                now_iso(),
            ),
        )
        return self.get_candidate(candidate_id)

    def get_candidate(self, candidate_id):
        conn = self.connect()
        return _row_to_dict(
            conn.execute(
                "SELECT * FROM candidates WHERE candidate_id=?", (candidate_id,)
            ).fetchone()
        )

    def list_candidates(self, task_id=None):
        conn = self.connect()
        if task_id:
            rows = conn.execute(
                "SELECT * FROM candidates WHERE task_id=? ORDER BY created_at DESC",
                (task_id,),
            ).fetchall()
        else:
            rows = conn.execute(
                "SELECT * FROM candidates ORDER BY created_at DESC LIMIT 100"
            ).fetchall()
        return [_row_to_dict(r) for r in rows]

    def list_leases(self):
        conn = self.connect()
        return [_row_to_dict(r) for r in conn.execute("SELECT * FROM leases")]

    # -- diagnosi ---------------------------------------------------------

    def stats(self):
        conn = self.connect()

        def count(sql, args=()):
            return int(conn.execute(sql, args).fetchone()[0])

        return {
            "sessionsActive": count("SELECT COUNT(*) FROM sessions WHERE status='ACTIVE'"),
            "sessionsTotal": count("SELECT COUNT(*) FROM sessions"),
            "tasks": count("SELECT COUNT(*) FROM tasks"),
            "events": count("SELECT COUNT(*) FROM events"),
            "deliveriesPending": count(
                "SELECT COUNT(*) FROM deliveries WHERE state='PENDING'"
            ),
            "deliveriesAcked": count(
                "SELECT COUNT(*) FROM deliveries WHERE state='ACKED'"
            ),
            "candidates": count("SELECT COUNT(*) FROM candidates"),
        }


def open_store(path):
    """Apre e migra lo store. E' il solo punto d'ingresso che il daemon usa."""
    store = Store(path)
    store.migrate()
    return store
