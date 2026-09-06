"""`rt3d`: il coordinator locale.

E' l'UNICO processo che apre il database. Le sessioni non si parlano fra loro e non
aprono `runtime.db`: parlano con questo, e questo parla col database.

    Session A ──▶ rt3d ──▶ Session B

Ascolta su `127.0.0.1` e su una porta EFFIMERA, che pubblica in `daemon.json`. La porta
fissa sarebbe piu' comoda da documentare e sbagliata da usare: su questa workstation
sono gia' in ascolto tre VS Code, un Unreal e il ponte MCP, e una collisione di porta si
manifesta come "il daemon non parte" senza dire da cosa.

⚠️ Nessuna autenticazione, per scelta: e' localhost single-user, e la specifica lo
esclude esplicitamente dalla milestone. Il bind e' pero' su `127.0.0.1` e non su
`0.0.0.0`, che e' la differenza fra "senza autenticazione sulla mia macchina" e "senza
autenticazione sulla rete".
"""

import json
import os
import socket
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from . import PROTOCOL_VERSION, ROADMAP_SCHEMA_VERSION, SCHEMA_VERSION
from .errors import Rt3Error
from .model import now_iso
from .paths import daemon_file, db_path, ensure_store_root
from .store import open_store

PROTOCOL_HEADER = "X-RT3-Protocol"


# ---------------------------------------------------------------------------
# Operazioni RPC
# ---------------------------------------------------------------------------


def _ops(store, server):
    """Tabella delle operazioni. Chiave = `op` della richiesta.

    Una tabella e non un router REST: le operazioni sono verbi di dominio (`inbox.ack`)
    e non risorse, e forzarle in una gerarchia di path avrebbe aggiunto codice senza
    aggiungere significato.
    """

    def session_start(a):
        return store.start_session(
            session_id=a["sessionId"],
            role=a["role"],
            workspace_group=a["workspaceGroup"],
            lane=a["lane"],
            worktree_path=a.get("worktreePath"),
            repo_root=a.get("repoRoot"),
            branch=a.get("branch"),
            head=a.get("head"),
            task_id=a.get("taskId"),
            candidate_id=a.get("candidateId"),
            write_set=a.get("writeSet"),
            write_mode=a.get("writeMode", "READ_ONLY"),
            unreal_lease=a.get("unrealLease", "NONE"),
            host=a.get("host"),
            client_pid=a.get("clientPid"),
            replace=bool(a.get("replace")),
        )

    def _roadmap_view(roadmap_id=None):
        """Ricostruisce (Roadmap, Graph, progress) dal documento salvato.

        ⛔ Legge il DATABASE, non il file. Il file vive in un checkout, e i tre
        workspace ne hanno tre copie che possono divergere: se il daemon rileggesse il
        disco, la risposta dipenderebbe da quale checkout ha avviato il daemon. Cio'
        che e' stato caricato e' cio' che vale, per tutti e tre.
        """
        from .graph import build
        from .roadmap import Roadmap

        row = store.get_roadmap(roadmap_id, required=True)
        document = json.loads(row["document"])
        roadmap = Roadmap.from_dict(document)
        return row, roadmap, build(roadmap), store.progress_map(row["roadmap_id"])

    def roadmap_ready(a):
        from .planner import readiness

        row, roadmap, graph, progress = _roadmap_view(a.get("roadmapId"))
        result = readiness(roadmap, graph, progress)
        return {
            "roadmapId": row["roadmap_id"],
            "contentHash": row["content_hash"],
            "items": [
                {
                    "key": r.key,
                    "state": r.state,
                    "progress": r.progress,
                    "blockedBy": r.blocked_by,
                    "unmet": r.unmet,
                    "epicId": roadmap.items[r.key].epic_id,
                    "executionWork": roadmap.items[r.key].execution_work,
                    "estimate": roadmap.items[r.key].estimate,
                    "resources": roadmap.items[r.key].resources,
                }
                for r in result.values()
            ],
        }

    def roadmap_plan(a):
        from .planner import plan

        row, roadmap, graph, progress = _roadmap_view(a.get("roadmapId"))
        result = plan(roadmap, graph, progress)
        result["contentHash"] = row["content_hash"]
        return result

    def roadmap_graph(a):
        row, roadmap, graph, _progress = _roadmap_view(a.get("roadmapId"))
        payload = graph.as_dict()
        payload["roadmapId"] = row["roadmap_id"]
        payload["contentHash"] = row["content_hash"]
        return payload

    def roadmap_critical_path(a):
        from .graph import critical_path, schedule

        row, roadmap, graph, _progress = _roadmap_view(a.get("roadmapId"))
        path, duration = critical_path(graph)
        rows, _ = schedule(graph)
        return {
            "roadmapId": row["roadmap_id"],
            "contentHash": row["content_hash"],
            "duration": duration,
            "path": path,
            "rows": [r._asdict() for r in rows.values()],
        }

    def roadmap_summary(a):
        from .planner import summary

        row, roadmap, graph, progress = _roadmap_view(a.get("roadmapId"))
        payload = summary(roadmap, graph, progress)
        payload["contentHash"] = row["content_hash"]
        payload["name"] = row["name"]
        return payload

    def roadmap_set_state(a):
        """Avanza una issue. Rifiuta una chiave che la roadmap non contiene.

        Senza il controllo, un refuso (`B22` invece di `B2`) verrebbe salvato e non
        avrebbe alcun effetto sul planner: la issue vera resterebbe PENDING, e chi ha
        scritto il comando lo vedrebbe riuscire.
        """
        from .errors import ItemNotFound

        row, roadmap, _graph, _progress = _roadmap_view(a.get("roadmapId"))
        key = a["itemKey"]
        if key not in roadmap.items:
            short = [k for k in roadmap.items if k.rsplit("/", 1)[-1] == key]
            if len(short) == 1:
                key = short[0]
            else:
                raise ItemNotFound(
                    "la roadmap {} non contiene la issue {!r}{}.".format(
                        row["roadmap_id"],
                        a["itemKey"],
                        " (candidate: {})".format(", ".join(sorted(short)))
                        if short
                        else "",
                    )
                )
        return store.set_item_state(
            row["roadmap_id"],
            key,
            a["progress"],
            session_id=a.get("sessionId"),
            candidate_id=a.get("candidateId"),
            note=a.get("note"),
        )

    def roadmap_states(a):
        row = store.get_roadmap(a.get("roadmapId"), required=True)
        return {
            "roadmapId": row["roadmap_id"],
            "states": store.item_states(row["roadmap_id"]),
        }

    return {
        "health": lambda a: health_payload(store),
        "session.start": session_start,
        "session.get": lambda a: store.get_session(a["sessionId"], required=True),
        "session.update": lambda a: store.update_session(
            a["sessionId"],
            **{k: v for k, v in a.items() if k != "sessionId"},
        ),
        "session.stop": lambda a: store.stop_session(a["sessionId"]),
        "session.touch": lambda a: store.touch_session(a["sessionId"]),
        "sessions.list": lambda a: store.list_sessions(
            include_stopped=bool(a.get("includeStopped"))
        ),
        "event.publish": lambda a: store.publish(
            sender_session_id=a["sessionId"],
            event_type=a["type"],
            task_id=a.get("taskId"),
            candidate_id=a.get("candidateId"),
            payload=a.get("payload"),
            note=a.get("note"),
            recipient_session_id=a.get("toSession"),
            recipient_role=a.get("toRole"),
            recipient_lane=a.get("toLane"),
        ),
        "events.list": lambda a: store.list_events(
            limit=int(a.get("limit", 50)), task_id=a.get("taskId")
        ),
        "event.get": lambda a: store.get_event(a["eventId"]),
        "inbox.list": lambda a: store.inbox(
            a["sessionId"], state=a.get("state", "PENDING"), limit=int(a.get("limit", 100))
        ),
        "inbox.show": lambda a: store.show(a["sessionId"], a["ref"]),
        "inbox.ack": lambda a: store.ack(a["sessionId"], a["ref"], note=a.get("note")),
        "inbox.count": lambda a: {"pending": store.pending_count(a["sessionId"])},
        "task.get": lambda a: store.get_task(a["taskId"], required=True),
        "task.upsert": lambda a: store.upsert_task(
            a["taskId"], title=a.get("title"), status=a.get("status"), lane=a.get("lane")
        ),
        "tasks.list": lambda a: store.list_tasks(),
        "candidate.create": lambda a: store.create_candidate(
            a["sessionId"],
            task_id=a.get("taskId"),
            branch=a.get("branch"),
            head=a.get("head"),
            note=a.get("note"),
            roadmap_id=a.get("roadmapId"),
            item_key=a.get("itemKey"),
        ),
        "candidate.get": lambda a: store.get_candidate(a["candidateId"]),
        "candidate.setStatus": lambda a: store.set_candidate_status(
            a["candidateId"],
            a["status"],
            session_id=a.get("sessionId"),
            note=a.get("note"),
        ),
        "candidates.list": lambda a: store.list_candidates(task_id=a.get("taskId")),
        # -- roadmap orchestration
        "roadmap.save": lambda a: store.save_roadmap(
            roadmap_id=a["roadmapId"],
            name=a.get("name"),
            source_path=a.get("sourcePath"),
            content_hash=a["contentHash"],
            roadmap_schema_version=a["roadmapSchemaVersion"],
            document=a["document"],
            session_id=a.get("sessionId"),
            reset_state=bool(a.get("resetState")),
        ),
        "roadmap.get": lambda a: store.get_roadmap(a.get("roadmapId"), required=True),
        "roadmaps.list": lambda a: store.list_roadmaps(),
        "roadmap.states": roadmap_states,
        "roadmap.setState": roadmap_set_state,
        "roadmap.ready": roadmap_ready,
        "roadmap.plan": roadmap_plan,
        "roadmap.graph": roadmap_graph,
        "roadmap.criticalPath": roadmap_critical_path,
        "roadmap.summary": roadmap_summary,
        "leases.list": lambda a: store.list_leases(),
        "stats": lambda a: store.stats(),
        "daemon.stop": lambda a: server.request_stop(),
    }


def health_payload(store):
    return {
        "ok": True,
        "protocolVersion": PROTOCOL_VERSION,
        "schemaVersion": SCHEMA_VERSION,
        # La terza versione viaggia nell'health perche' la domanda «i tre workspace
        # sono compatibili?» si pone su tutte e tre insieme, e un client che dovesse
        # chiederla con una chiamata separata potrebbe leggerne due di momenti diversi.
        "roadmapSchemaVersion": ROADMAP_SCHEMA_VERSION,
        "dbSchemaVersion": store.schema_version(),
        "db": store.path,
        "pid": os.getpid(),
        "time": now_iso(),
    }


# ---------------------------------------------------------------------------
# Server
# ---------------------------------------------------------------------------


class _Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    server_version = "rt3d/{}".format(PROTOCOL_VERSION)

    def log_message(self, fmt, *args):  # pragma: no cover - rumore
        """Silenzia il log per-richiesta di BaseHTTPRequestHandler.

        Va su stderr riga per riga e, in un daemon avviato senza console, riempie il
        file di log con traffico che nessuno legge. Gli errori veri passano da
        `_send`.
        """

    def _send(self, status, body):
        raw = json.dumps(body).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.send_header(PROTOCOL_HEADER, str(PROTOCOL_VERSION))
        self.end_headers()
        self.wfile.write(raw)

    def do_GET(self):
        if self.path.rstrip("/") in ("/health", ""):
            self._send(200, {"ok": True, "result": health_payload(self.server.store)})
        else:
            self._send(404, {"ok": False, "code": "RT3_NO_ROUTE", "error": self.path})

    def do_POST(self):
        if self.path.rstrip("/") != "/rpc":
            self._send(404, {"ok": False, "code": "RT3_NO_ROUTE", "error": self.path})
            return

        # Il controllo di protocollo viene PRIMA di leggere il corpo: se il client
        # parla un'altra versione, il corpo potrebbe avere una forma che non sappiamo
        # interpretare, e interpretarlo comunque sarebbe la degradazione silenziosa che
        # la versione esiste per impedire.
        client_protocol = self.headers.get(PROTOCOL_HEADER)
        if client_protocol is not None and client_protocol.strip() != str(
            PROTOCOL_VERSION
        ):
            self._send(
                409,
                {
                    "ok": False,
                    "code": "RT3_PROTOCOL_MISMATCH",
                    "error": (
                        "il client parla protocollo RT3 v{}, questo rt3d parla v{}. "
                        "I workspace devono eseguire la stessa versione del control "
                        "plane: allineare i checkout e riavviare rt3d.".format(
                            client_protocol.strip(), PROTOCOL_VERSION
                        )
                    ),
                    "daemonProtocolVersion": PROTOCOL_VERSION,
                },
            )
            return

        try:
            length = int(self.headers.get("Content-Length") or 0)
            raw = self.rfile.read(length) if length else b"{}"
            request = json.loads(raw.decode("utf-8") or "{}")
        except (ValueError, OSError) as exc:
            self._send(
                400, {"ok": False, "code": "RT3_BAD_REQUEST", "error": str(exc)}
            )
            return

        op = request.get("op")
        args = request.get("args") or {}
        handler = self.server.ops.get(op)
        if handler is None:
            self._send(
                400,
                {
                    "ok": False,
                    "code": "RT3_UNKNOWN_OP",
                    "error": "operazione sconosciuta: {!r}".format(op),
                },
            )
            return

        try:
            result = handler(args)
        except Rt3Error as exc:
            self._send(
                400, {"ok": False, "code": exc.code, "error": exc.message}
            )
        except KeyError as exc:
            self._send(
                400,
                {
                    "ok": False,
                    "code": "RT3_MISSING_ARGUMENT",
                    "error": "argomento mancante per {}: {}".format(op, exc),
                },
            )
        except Exception as exc:  # pragma: no cover - imprevisto
            # Un imprevisto non deve uccidere il daemon: gli altri due terminali stanno
            # ancora lavorando. Torna 500 con il tipo dell'eccezione, e il traceback
            # finisce nel log del daemon.
            import traceback

            traceback.print_exc(file=sys.stderr)
            self._send(
                500,
                {
                    "ok": False,
                    "code": "RT3_INTERNAL",
                    "error": "{}: {}".format(type(exc).__name__, exc),
                },
            )
        else:
            self._send(200, {"ok": True, "result": result})


class Rt3Server(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True

    def __init__(self, addr, store):
        super().__init__(addr, _Handler)
        self.store = store
        self.ops = _ops(store, self)

    def request_stop(self):
        """Ferma il server dopo aver risposto. Va su un thread: `shutdown()` chiamato
        dal thread che sta servendo la richiesta si autoblocca."""
        threading.Thread(target=self.shutdown, daemon=True).start()
        return {"stopping": True, "pid": os.getpid()}


def write_daemon_file(host, port):
    payload = {
        "host": host,
        "port": port,
        "pid": os.getpid(),
        "protocolVersion": PROTOCOL_VERSION,
        "schemaVersion": SCHEMA_VERSION,
        "db": db_path(),
        "startedAt": now_iso(),
    }
    path = daemon_file()
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as fh:
        json.dump(payload, fh, indent=2)
    os.replace(tmp, path)
    return payload


def read_daemon_file():
    try:
        with open(daemon_file(), "r", encoding="utf-8") as fh:
            return json.load(fh)
    except (OSError, ValueError):
        return None


def clear_daemon_file(expected_pid=None):
    """Rimuove `daemon.json`, ma solo se descrive QUESTO processo.

    ⚠️ Senza il controllo sul pid, un daemon che esce mentre un altro e' gia' partito
    cancellerebbe l'endpoint del vivo, e i tre terminali smetterebbero di trovarlo.
    """
    info = read_daemon_file()
    if info is None:
        return
    if expected_pid is not None and info.get("pid") != expected_pid:
        return
    try:
        os.remove(daemon_file())
    except OSError:
        pass


def probe(timeout=1.5):
    """Il daemon descritto da `daemon.json` risponde? Ritorna il payload o None.

    Un `daemon.json` che resta dopo un crash e' lo stato normale, non un caso limite: e'
    quello che si ottiene chiudendo la finestra del terminale che ospitava rt3d. Per
    questo la presenza del file non prova niente, e si verifica sempre con una chiamata.
    """
    from .client import Client  # import locale: evita il ciclo client <-> daemon

    info = read_daemon_file()
    if info is None:
        return None
    try:
        return Client(info["host"], info["port"], timeout=timeout).health()
    except Exception:
        return None


def serve(host="127.0.0.1", port=0, ready_callback=None):
    """Avvia il daemon in foreground. Ritorna quando riceve `daemon.stop`."""
    ensure_store_root()
    store = open_store(db_path())
    server = Rt3Server((host, port), store)
    actual_host, actual_port = server.server_address[0], server.server_address[1]
    info = write_daemon_file(actual_host, actual_port)
    if ready_callback:
        ready_callback(info)
    try:
        server.serve_forever(poll_interval=0.2)
    finally:
        clear_daemon_file(expected_pid=os.getpid())
        server.server_close()
        store.close()
    return info


def start_background(python_executable=None, wait_seconds=15.0):
    """Avvia `rt3d` come processo staccato e attende che risponda.

    Staccato e non figlio: il daemon deve sopravvivere alla chiusura del terminale che
    lo ha avviato, altrimenti il primo dei tre VS Code che si chiude porta via il
    coordinator degli altri due.
    """
    import subprocess

    existing = probe()
    if existing is not None:
        return {"alreadyRunning": True, "health": existing, "info": read_daemon_file()}

    # Un `daemon.json` stantio (probe fallito) va tolto ORA: se restasse, e il nuovo
    # daemon non partisse, la diagnosi punterebbe a un endpoint morto.
    clear_daemon_file()

    ensure_store_root()
    python_executable = python_executable or sys.executable
    package_parent = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    log = open(os.path.join(os.path.dirname(daemon_file()), "rt3d.log"), "ab")

    creationflags = 0
    start_new_session = False
    if os.name == "nt":
        # DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP: niente console ereditata, e
        # Ctrl+C nel terminale che lo ha avviato non lo uccide.
        creationflags = 0x00000008 | 0x00000200
    else:
        start_new_session = True

    env = dict(os.environ)
    env["PYTHONPATH"] = (
        package_parent + os.pathsep + env["PYTHONPATH"]
        if env.get("PYTHONPATH")
        else package_parent
    )
    env["PYTHONIOENCODING"] = "utf-8"

    proc = subprocess.Popen(
        [python_executable, "-m", "rt3", "daemon", "run"],
        cwd=package_parent,
        stdout=log,
        stderr=log,
        stdin=subprocess.DEVNULL,
        env=env,
        creationflags=creationflags,
        start_new_session=start_new_session,
    )

    deadline = time.time() + wait_seconds
    while time.time() < deadline:
        health = probe(timeout=0.7)
        if health is not None:
            return {
                "alreadyRunning": False,
                "health": health,
                "info": read_daemon_file(),
                "pid": proc.pid,
            }
        if proc.poll() is not None:
            break
        time.sleep(0.15)

    return {
        "alreadyRunning": False,
        "health": None,
        "info": read_daemon_file(),
        "pid": proc.pid,
        "exitCode": proc.poll(),
    }


def free_port():  # pragma: no cover - utilita' per i test
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port
