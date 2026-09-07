"""CLI `rt3`.

Ogni comando che tocca lo stato passa dal daemon. La CLI non apre mai il database.

Output: leggibile per default, `--json` quando a leggere e' uno script. Gli errori
previsti escono su stderr nella forma `CODICE: messaggio` con exit code dedicato; un
traceback compare solo per un imprevisto vero, che e' l'informazione che serve quando
compare.

⚠️ Testo ASCII di proposito. La console di Windows in cp1252 non stampa le frecce
unicode, e un `UnicodeEncodeError` durante un handoff e' un guasto che sembra del
control plane e non lo e'.
"""

import argparse
import json as jsonlib
import os
import sys

from . import PROTOCOL_VERSION, ROADMAP_SCHEMA_VERSION, SCHEMA_VERSION
from .errors import Rt3Error
from .gitmeta import collect as collect_git
from .gitmeta import short_head
from .model import (
    CANDIDATE_STATUSES,
    EVENT_TYPES,
    ITEM_MODES,
    ITEM_PROGRESS_STATES,
    LANES,
    ROLES,
    WORKSPACE_GROUPS,
    WRITE_MODES,
)

# ---------------------------------------------------------------------------
# Stampa
# ---------------------------------------------------------------------------


def out(text=""):
    sys.stdout.write(str(text) + "\n")


def emit(args, payload, render):
    if getattr(args, "json", False):
        out(jsonlib.dumps(payload, indent=2, ensure_ascii=False))
    else:
        render(payload)


def _dash(value):
    if value is None or value == "":
        return "-"
    return str(value)


def _branch(value):
    """Un branch assente e' detached HEAD, non un dato mancante: si legge diverso."""
    return value if value else "(detached)"


def _table(rows, headers):
    if not rows:
        return
    widths = [len(h) for h in headers]
    for row in rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(str(cell)))
    fmt = "  ".join("{{:<{}}}".format(w) for w in widths)
    out(fmt.format(*headers))
    out(fmt.format(*["-" * w for w in widths]))
    for row in rows:
        out(fmt.format(*[str(c) for c in row]))


# ---------------------------------------------------------------------------
# Contesto
# ---------------------------------------------------------------------------


def _client(args):
    from .client import connect

    return connect(timeout=float(getattr(args, "timeout", 30.0)))


def _session_id(args, required=True):
    from .binding import resolve

    return resolve(getattr(args, "session", None), required=required)


# ---------------------------------------------------------------------------
# daemon
# ---------------------------------------------------------------------------


def cmd_daemon_run(args):
    from .daemon import serve

    def ready(info):
        out(
            "rt3d in ascolto su {}:{} (pid {}) - protocollo v{}, schema v{}".format(
                info["host"],
                info["port"],
                info["pid"],
                info["protocolVersion"],
                info["schemaVersion"],
            )
        )
        out("database: {}".format(info["db"]))
        sys.stdout.flush()

    serve(host=args.host, port=args.port, ready_callback=ready)
    return 0


def cmd_daemon_start(args):
    from .daemon import start_background

    result = start_background()
    if result.get("health") is None:
        raise Rt3Error(
            "rt3d non ha risposto entro il tempo previsto. Log: {}. Provare "
            "`rt3 daemon run` in primo piano per vedere l'errore.".format(
                os.path.join(os.path.dirname(_daemon_file()), "rt3d.log")
            ),
            code="RT3_DAEMON_START_FAILED",
            exit_code=3,
        )

    def render(payload):
        info = payload["info"]
        if payload["alreadyRunning"]:
            out(
                "rt3d era gia' in esecuzione: {}:{} (pid {})".format(
                    info["host"], info["port"], info["pid"]
                )
            )
        else:
            out("rt3d avviato: {}:{} (pid {})".format(info["host"], info["port"], info["pid"]))
        out("database: {}".format(info["db"]))
        out(
            "protocollo v{}, schema v{}".format(
                info["protocolVersion"], info["schemaVersion"]
            )
        )

    emit(args, result, render)
    return 0


def _daemon_file():
    from .paths import daemon_file

    return daemon_file()


def cmd_daemon_status(args):
    from .daemon import probe, read_daemon_file

    info = read_daemon_file()
    health = probe()
    payload = {"info": info, "health": health, "running": health is not None}

    def render(p):
        if p["running"]:
            out(
                "rt3d ATTIVO su {}:{} (pid {})".format(
                    p["info"]["host"], p["info"]["port"], p["health"]["pid"]
                )
            )
            out("database: {}".format(p["health"]["db"]))
            out(
                "protocollo v{} | schema codice v{} | schema database v{}".format(
                    p["health"]["protocolVersion"],
                    p["health"]["schemaVersion"],
                    p["health"]["dbSchemaVersion"],
                )
            )
        elif p["info"] is not None:
            out(
                "rt3d NON risponde, ma {} esiste e descrive {}:{} (pid {}).".format(
                    _daemon_file(),
                    p["info"].get("host"),
                    p["info"].get("port"),
                    p["info"].get("pid"),
                )
            )
            out("Endpoint stantio: `rt3 daemon start` lo rimuove e ne avvia uno nuovo.")
        else:
            out("rt3d non avviato. `rt3 daemon start` per avviarlo.")

    emit(args, payload, render)
    return 0 if payload["running"] else 3


def cmd_daemon_stop(args):
    from .daemon import probe

    if probe() is None:
        out("rt3d non era in esecuzione.")
        return 0
    client = _client(args)
    result = client.call("daemon.stop")
    emit(args, result, lambda p: out("rt3d fermato (pid {}).".format(p.get("pid"))))
    return 0


def cmd_daemon_restart(args):
    cmd_daemon_stop(args)
    import time

    from .daemon import probe

    for _ in range(40):
        if probe(timeout=0.4) is None:
            break
        time.sleep(0.15)
    return cmd_daemon_start(args)


# ---------------------------------------------------------------------------
# session
# ---------------------------------------------------------------------------


def cmd_session_start(args):
    from .binding import bind

    client = _client(args)
    git = collect_git(args.worktree or os.getcwd())

    payload = {
        "sessionId": args.id,
        "role": args.role,
        "workspaceGroup": args.workspace_group,
        "lane": args.lane,
        "worktreePath": args.worktree or git["worktreePath"] or os.getcwd(),
        "repoRoot": git["repoRoot"],
        "branch": git["branch"],
        "head": git["head"],
        "taskId": args.task,
        "writeMode": args.write_mode,
        "writeSet": args.write_set or [],
        "clientPid": os.getpid(),
        "host": _hostname(),
        "replace": bool(args.replace),
    }
    session = client.call("session.start", **payload)
    binding_path = bind(session["session_id"])
    result = {"session": session, "binding": binding_path, "git": git}

    def render(p):
        s = p["session"]
        out("sessione {} registrata.".format(s["session_id"]))
        out("  role/lane      : {} / {}".format(s["role"], s["lane"]))
        out("  workspaceGroup : {}".format(s["workspace_group"]))
        out("  worktree       : {}".format(_dash(s["worktree_path"])))
        out("  repoRoot       : {}".format(_dash(s["repo_root"])))
        out(
            "  branch @ HEAD  : {} @ {}".format(
                _branch(s["branch"]), short_head(s["head"])
            )
        )
        out("  task           : {}".format(_dash(s["task_id"])))
        out("  writeMode      : {}".format(s["write_mode"]))
        out("  binding        : {}".format(p["binding"]))
        if git["repoRoot"] is None:
            out(
                "  ! nessun repository Git in questa directory: i metadati Git restano "
                "vuoti (previsto)."
            )
        elif git["branch"] is None:
            out("  ! detached HEAD: il campo branch resta vuoto (previsto).")

    emit(args, result, render)
    return 0


def _hostname():
    import socket

    try:
        return socket.gethostname()
    except OSError:  # pragma: no cover
        return None


def cmd_session_status(args):
    client = _client(args)
    session_id, origin = _session_id(args)
    session = client.call("session.get", sessionId=session_id)
    pending = client.call("inbox.count", sessionId=session_id)
    result = {"session": session, "pending": pending["pending"], "origin": origin}

    def render(p):
        s = p["session"]
        out("sessione {} ({})".format(s["session_id"], s["status"]))
        out("  origine id     : {}".format(p["origin"]))
        out("  role/lane      : {} / {}".format(s["role"], s["lane"]))
        out("  workspaceGroup : {}".format(s["workspace_group"]))
        out("  worktree       : {}".format(_dash(s["worktree_path"])))
        out(
            "  branch @ HEAD  : {} @ {}".format(
                _branch(s["branch"]), short_head(s["head"])
            )
        )
        out("  task           : {}".format(_dash(s["task_id"])))
        out("  writeMode      : {}".format(s["write_mode"]))
        out("  unrealLease    : {}".format(s["unreal_lease"]))
        out("  startedAt      : {}".format(s["started_at"]))
        out("  lastSeenAt     : {}".format(s["last_seen_at"]))
        out("  eventi pending : {}".format(p["pending"]))

    emit(args, result, render)
    return 0


def cmd_session_set(args):
    client = _client(args)
    session_id, _ = _session_id(args)
    fields = {"sessionId": session_id}
    if args.task:
        fields["task_id"] = args.task
    if args.write_mode:
        fields["write_mode"] = args.write_mode
    if args.unreal_lease:
        fields["unreal_lease"] = args.unreal_lease
    if args.refresh_git:
        git = collect_git(os.getcwd())
        fields["branch"] = git["branch"]
        fields["head"] = git["head"]
    session = client.call("session.update", **fields)
    emit(
        args,
        session,
        lambda s: out(
            "sessione {} aggiornata: task={} writeMode={} branch={} head={}".format(
                s["session_id"],
                _dash(s["task_id"]),
                s["write_mode"],
                _branch(s["branch"]),
                short_head(s["head"]),
            )
        ),
    )
    return 0


def cmd_session_stop(args):
    from .binding import unbind

    client = _client(args)
    session_id, _ = _session_id(args)
    session = client.call("session.stop", sessionId=session_id)
    unbind()
    emit(
        args,
        session,
        lambda s: out(
            "sessione {} fermata alle {}. Binding del terminale rimosso.".format(
                s["session_id"], s["stopped_at"]
            )
        ),
    )
    return 0


def cmd_sessions_list(args):
    client = _client(args)
    sessions = client.call("sessions.list", includeStopped=bool(args.all))

    def render(rows):
        if not rows:
            out("nessuna sessione registrata.")
            return
        _table(
            [
                [
                    s["session_id"],
                    s["role"],
                    s["lane"],
                    s["workspace_group"],
                    s["status"],
                    _dash(s["task_id"]),
                    _branch(s["branch"]),
                    short_head(s["head"]),
                    s["write_mode"],
                    s["last_seen_at"],
                ]
                for s in rows
            ],
            [
                "SESSION",
                "ROLE",
                "LANE",
                "WSGROUP",
                "STATUS",
                "TASK",
                "BRANCH",
                "HEAD",
                "WRITE",
                "LAST SEEN",
            ],
        )

    emit(args, sessions, render)
    return 0


# ---------------------------------------------------------------------------
# eventi e mailbox
# ---------------------------------------------------------------------------


def cmd_event_publish(args):
    client = _client(args)
    session_id, _ = _session_id(args)

    payload = {}
    if args.payload:
        try:
            payload = jsonlib.loads(args.payload)
        except ValueError as exc:
            raise Rt3Error(
                "--payload non e' JSON valido: {}".format(exc),
                code="RT3_INVALID_EVENT",
                exit_code=9,
            )
        if not isinstance(payload, dict):
            raise Rt3Error(
                "--payload deve essere un oggetto JSON, non {}.".format(
                    type(payload).__name__
                ),
                code="RT3_INVALID_EVENT",
                exit_code=9,
            )

    # Il control plane esiste per non far ricopiare i dati a mano: branch e HEAD della
    # sessione entrano nel payload da soli, salvo che il chiamante li abbia gia' messi.
    if args.with_git:
        git = collect_git(os.getcwd())
        payload.setdefault("branch", git["branch"])
        payload.setdefault("head", git["head"])
        payload.setdefault("worktreePath", git["worktreePath"])

    result = client.call(
        "event.publish",
        sessionId=session_id,
        type=args.type,
        taskId=args.task,
        candidateId=args.candidate,
        payload=payload,
        note=args.note,
        toSession=args.to_session,
        toRole=args.to_role,
        toLane=args.to_lane,
    )

    def render(p):
        out("evento {} pubblicato: {}".format(p["eventId"], p["type"]))
        if p["recipients"]:
            for r in p["recipients"]:
                if r["session_id"]:
                    out("  -> sessione {}".format(r["session_id"]))
                else:
                    out("  -> {} della lane {}".format(r["role"], r["lane"]))
            out("  regola: {}".format(p["routingRule"]))
        else:
            # Non e' un errore, ed e' importante che non lo sembri: l'evento e' nel log
            # e resta consultabile. Ma nessuno lo ricevera', e tacerlo sarebbe la
            # peggiore delle due opzioni.
            out("  ! NESSUN DESTINATARIO: evento registrato, non consegnato.")
            out("    {}".format(p["routingReason"]))

    emit(args, result, render)
    return 0


def cmd_inbox_list(args):
    client = _client(args)
    session_id, _ = _session_id(args)
    state = "ALL" if args.all else "PENDING"
    rows = client.call("inbox.list", sessionId=session_id, state=state, limit=args.limit)

    def render(items):
        if not items:
            out("nessun evento {} per {}.".format(state.lower(), session_id))
            return
        _table(
            [
                [
                    r["delivery_id"],
                    r["event_id"],
                    r["type"],
                    r["sender_session_id"],
                    "{}/{}".format(r["sender_role"], r["sender_lane"]),
                    _dash(r["task_id"]),
                    r["state"],
                    r["event_created_at"],
                ]
                for r in items
            ],
            ["DELIVERY", "EVENT", "TYPE", "FROM", "ROLE/LANE", "TASK", "STATE", "AT"],
        )
        out("")
        out("`rt3 inbox show <EVENT>` per il dettaglio, `rt3 inbox ack <EVENT>` per prenderlo.")

    emit(args, rows, render)
    return 0


def cmd_inbox_show(args):
    client = _client(args)
    session_id, _ = _session_id(args)
    result = client.call("inbox.show", sessionId=session_id, ref=args.ref)

    def render(p):
        e, d = p["event"], p["delivery"]
        out("evento {} - {}".format(e["event_id"], e["type"]))
        out("  quando      : {}".format(e["created_at"]))
        out(
            "  mittente    : {} ({} sulla lane {})".format(
                e["sender_session_id"], e["sender_role"], e["sender_lane"]
            )
        )
        if d["recipient_session_id"]:
            out("  destinatario: sessione {}".format(d["recipient_session_id"]))
        else:
            out(
                "  destinatario: {} della lane {}".format(
                    d["recipient_role"], d["recipient_lane"]
                )
            )
        out("  task        : {}".format(_dash(e["task_id"])))
        out("  candidate   : {}".format(_dash(e["candidate_id"])))
        out("  regola      : {}".format(_dash(e["routing_rule"])))
        out("  stato       : {}".format(d["state"]))
        if d["state"] == "ACKED":
            out("  ack         : {} alle {}".format(d["acked_by"], d["acked_at"]))
        if e.get("note"):
            out("  nota        : {}".format(e["note"]))
        if e.get("payload"):
            out("  payload     :")
            for line in jsonlib.dumps(e["payload"], indent=2, ensure_ascii=False).splitlines():
                out("    " + line)

    emit(args, result, render)
    return 0


def cmd_inbox_ack(args):
    client = _client(args)
    session_id, _ = _session_id(args)
    result = client.call(
        "inbox.ack", sessionId=session_id, ref=args.ref, note=args.note
    )
    emit(
        args,
        result,
        lambda d: out(
            "consegna {} (evento {}) presa da {} alle {}.".format(
                d["delivery_id"], d["event_id"], d["acked_by"], d["acked_at"]
            )
        ),
    )
    return 0


def cmd_events_list(args):
    client = _client(args)
    rows = client.call("events.list", limit=args.limit, taskId=args.task)

    def render(items):
        if not items:
            out("event log vuoto.")
            return
        _table(
            [
                [
                    r["seq"],
                    r["event_id"],
                    r["type"],
                    r["sender_session_id"],
                    _dash(r["task_id"]),
                    _dash(r["routing_rule"]),
                    r["created_at"],
                ]
                for r in items
            ],
            ["SEQ", "EVENT", "TYPE", "FROM", "TASK", "RULE", "AT"],
        )

    emit(args, rows, render)
    return 0


# ---------------------------------------------------------------------------
# task e candidate
# ---------------------------------------------------------------------------


def cmd_task_show(args):
    client = _client(args)
    task = client.call("task.get", taskId=args.task_id)

    def render(t):
        out("task {} [{}]".format(t["task_id"], t["status"]))
        out("  titolo    : {}".format(_dash(t.get("title"))))
        out("  lane      : {}".format(_dash(t.get("lane"))))
        out("  creato    : {}".format(t["created_at"]))
        out("  aggiornato: {}".format(t["updated_at"]))
        out("")
        out("sessioni sul task:")
        if t["sessions"]:
            _table(
                [
                    [
                        s["session_id"],
                        s["role"],
                        s["lane"],
                        s["status"],
                        _branch(s["branch"]),
                        short_head(s["head"]),
                    ]
                    for s in t["sessions"]
                ],
                ["SESSION", "ROLE", "LANE", "STATUS", "BRANCH", "HEAD"],
            )
        else:
            out("  nessuna.")
        out("")
        out("eventi del task:")
        if t["events"]:
            _table(
                [
                    [e["seq"], e["event_id"], e["type"], e["sender_session_id"], e["created_at"]]
                    for e in t["events"]
                ],
                ["SEQ", "EVENT", "TYPE", "FROM", "AT"],
            )
        else:
            out("  nessuno.")

    emit(args, task, render)
    return 0


def cmd_tasks_list(args):
    client = _client(args)
    rows = client.call("tasks.list")

    def render(items):
        if not items:
            out("nessun task noto al control plane.")
            return
        _table(
            [
                [t["task_id"], t["status"], _dash(t.get("lane")), _dash(t.get("title")), t["updated_at"]]
                for t in items
            ],
            ["TASK", "STATUS", "LANE", "TITLE", "UPDATED"],
        )

    emit(args, rows, render)
    return 0


def cmd_candidate_create(args):
    client = _client(args)
    session_id, _ = _session_id(args)
    git = collect_git(os.getcwd())
    cand = client.call(
        "candidate.create",
        sessionId=session_id,
        taskId=args.task,
        branch=args.branch or git["branch"],
        head=args.head or git["head"],
        note=args.note,
        roadmapId=getattr(args, "roadmap_id", None),
        itemKey=getattr(args, "item_key", None),
    )
    emit(
        args,
        cand,
        lambda c: out(
            "candidate {} creato su {} @ {} (task {}).".format(
                c["candidate_id"],
                _branch(c["branch"]),
                short_head(c["head"]),
                _dash(c["task_id"]),
            )
        ),
    )
    return 0


def cmd_candidates_list(args):
    client = _client(args)
    rows = client.call("candidates.list", taskId=args.task)

    def render(items):
        if not items:
            out("nessun candidate.")
            return
        _table(
            [
                [
                    c["candidate_id"],
                    c.get("status") or "PENDING",
                    _dash(c.get("item_key")),
                    _branch(c.get("branch")),
                    short_head(c.get("head")),
                    _dash(c.get("session_id")),
                ]
                for c in items
            ],
            ["CANDIDATE", "STATUS", "ITEM", "BRANCH", "HEAD", "BY"],
        )

    emit(args, rows, render)
    return 0


def cmd_candidate_status(args):
    """Registra l'esito di UN candidate. E' il verdetto del validator.

    Deliberatamente separato da `roadmap state set`: passare un candidate non avanza da
    solo la issue, perche' un PASSED su un commit non dice che la issue sia finita. Chi
    vuole entrambe le cose fa due gesti, e ognuno resta leggibile nell'event log.
    """
    client = _client(args)
    session_id, _ = _session_id(args, required=False)
    cand = client.call(
        "candidate.setStatus",
        candidateId=args.candidate_id,
        status=args.status,
        sessionId=session_id,
        note=args.note,
    )
    emit(
        args,
        cand,
        lambda c: out(
            "candidate {} -> {} (item {}, {} @ {}).".format(
                c["candidate_id"],
                c["status"],
                _dash(c.get("item_key")),
                _branch(c.get("branch")),
                short_head(c.get("head")),
            )
        ),
    )
    return 0


# ---------------------------------------------------------------------------
# roadmap
# ---------------------------------------------------------------------------


def _print_problems(problems):
    """Stampa i rilievi della validazione. Ritorna il numero di ERROR."""
    errors = 0
    for p in problems:
        if p.level == "ERROR":
            errors += 1
        out("  {} {:<32} {}".format(p.level[0], p.code, p.message))
        out("      in {}".format(p.where))
    return errors


def cmd_roadmap_validate(args):
    """Valida un file. NON richiede il daemon.

    E' l'unico comando roadmap che gira offline, ed e' voluto: la validazione e' la
    cosa che si vuole poter fare mentre si scrive il file, prima che esista un control
    plane a cui darlo.
    """
    from .graph import build, cycle_problems
    from .roadmap import RoadmapError, load_document

    path = os.path.abspath(args.file)
    try:
        roadmap, problems = load_document(path)
    except RoadmapError as exc:
        out("roadmap NON valida: {}".format(path))
        _print_problems(exc.problems)
        return exc.exit_code

    if roadmap is not None:
        problems = list(problems) + cycle_problems(build(roadmap))

    errors = [p for p in problems if p.level == "ERROR"]
    warnings = [p for p in problems if p.level == "WARNING"]

    if getattr(args, "json", False):
        out(
            jsonlib.dumps(
                {
                    "path": path,
                    "valid": roadmap is not None and not errors,
                    "roadmapId": roadmap.id if roadmap else None,
                    "contentHash": roadmap.content_hash if roadmap else None,
                    "roadmapSchemaVersion": roadmap.schema_version if roadmap else None,
                    "items": len(roadmap.items) if roadmap else 0,
                    "epics": len(roadmap.epics) if roadmap else 0,
                    "problems": [p.as_dict() for p in problems],
                },
                indent=2,
                ensure_ascii=False,
            )
        )
        return 0 if (roadmap is not None and not errors) else 20

    if roadmap is None or errors:
        out("roadmap NON valida: {}".format(path))
        _print_problems(problems)
        return 20

    out("roadmap valida: {}".format(path))
    out("  id       : {}".format(roadmap.id))
    out("  nome     : {}".format(_dash(roadmap.name)))
    out("  schema   : v{}".format(roadmap.schema_version))
    out("  hash     : {}".format(roadmap.content_hash))
    out("  epic     : {}".format(len(roadmap.epics)))
    out("  issue    : {}".format(len(roadmap.items)))
    if warnings:
        out("  warning  : {}".format(len(warnings)))
        _print_problems(warnings)
    return 0


def cmd_roadmap_load(args):
    from .graph import load_checked
    from .roadmap import RoadmapError, dumps

    path = os.path.abspath(args.file)
    try:
        roadmap, _graph, problems = load_checked(path)
    except RoadmapError as exc:
        out("roadmap NON caricata: {}".format(exc.message))
        _print_problems(exc.problems)
        return exc.exit_code

    client = _client(args)
    session_id, _ = _session_id(args, required=False)
    saved = client.call(
        "roadmap.save",
        roadmapId=roadmap.id,
        name=roadmap.name,
        sourcePath=path,
        contentHash=roadmap.content_hash,
        roadmapSchemaVersion=roadmap.schema_version,
        document=dumps(roadmap),
        sessionId=session_id,
        resetState=bool(args.reset_state),
    )

    def render(row):
        out(
            "roadmap {} caricata: {} epic, {} issue, hash {}.".format(
                row["roadmap_id"],
                len(roadmap.epics),
                len(roadmap.items),
                row["content_hash"],
            )
        )
        if args.reset_state:
            out("  stato azzerato su richiesta (--reset-state).")
        warnings = [p for p in problems if p.level == "WARNING"]
        if warnings:
            out("  {} warning:".format(len(warnings)))
            _print_problems(warnings)

    emit(args, saved, render)
    return 0


def cmd_roadmaps_list(args):
    client = _client(args)
    rows = client.call("roadmaps.list")

    def render(items):
        if not items:
            out("nessuna roadmap caricata.")
            return
        _table(
            [
                [
                    r["roadmap_id"],
                    "v{}".format(r["roadmap_schema_version"]),
                    r["content_hash"],
                    r["items"],
                    r["loaded_at"],
                ]
                for r in items
            ],
            ["ROADMAP", "SCHEMA", "HASH", "STATI", "CARICATA"],
        )

    emit(args, rows, render)
    return 0


def cmd_roadmap_show(args):
    client = _client(args)
    payload = client.call("roadmap.summary", roadmapId=args.id)

    def render(s):
        out("roadmap {} ({})".format(s["roadmapId"], _dash(s.get("name"))))
        out("  hash : {}".format(s["contentHash"]))
        out("  epic : {}".format(s["epics"]))
        out("  issue: {}".format(s["items"]))
        out("")
        _table(
            [[state, count] for state, count in s["byState"].items()],
            ["STATE", "COUNT"],
        )

    emit(args, payload, render)
    return 0


def cmd_roadmap_graph(args):
    client = _client(args)
    payload = client.call("roadmap.graph", roadmapId=args.id)

    def render(g):
        out("grafo di {} - {} nodi, {} archi".format(
            g["roadmapId"], len(g["nodes"]), len(g["edges"])
        ))
        out("")
        _table(
            [
                [
                    n["key"],
                    n["estimate"],
                    ", ".join(
                        "{}({})".format(r["item"], r["gate"]) for r in n["requires"]
                    )
                    or "-",
                    ", ".join(n["unlocks"]) or "-",
                ]
                for n in g["nodes"]
            ],
            ["ITEM", "EST", "REQUIRES", "UNLOCKS"],
        )

    emit(args, payload, render)
    return 0


def cmd_roadmap_critical_path(args):
    client = _client(args)
    payload = client.call("roadmap.criticalPath", roadmapId=args.id)

    def render(p):
        out(
            "critical path di {} - durata {} (limite imposto dalle DIPENDENZE, non "
            "dalle risorse)".format(p["roadmapId"], p["duration"])
        )
        out("  " + (" -> ".join(p["path"]) if p["path"] else "(nessuno)"))
        out("")
        _table(
            [
                [
                    r["key"],
                    r["estimate"],
                    r["earliest_start"],
                    r["earliest_finish"],
                    r["latest_start"],
                    r["slack"],
                    "SI" if r["critical"] else "",
                ]
                for r in p["rows"]
            ],
            ["ITEM", "EST", "ES", "EF", "LS", "SLACK", "CRIT"],
        )

    emit(args, payload, render)
    return 0


def cmd_roadmap_ready(args):
    client = _client(args)
    payload = client.call("roadmap.ready", roadmapId=args.id)

    def render(p):
        items = p["items"]
        if args.state:
            items = [i for i in items if i["state"] == args.state]
        out("readiness di {} ({} item)".format(p["roadmapId"], len(items)))
        out("")
        _table(
            [
                [
                    i["key"],
                    i["state"],
                    i["progress"],
                    _dash(i["executionWork"]),
                    ", ".join(i["resources"]) or "-",
                    ", ".join(
                        "{}<{}>".format(u["item"], u["actual"]) for u in i["unmet"]
                    )
                    or "-",
                ]
                for i in items
            ],
            ["ITEM", "STATE", "PROGRESS", "WORKSPACE", "RES", "ATTENDE"],
        )

    emit(args, payload, render)
    return 0


def cmd_roadmap_plan(args):
    client = _client(args)
    payload = client.call("roadmap.plan", roadmapId=args.id)

    def render(p):
        out("piano di {}".format(p["roadmapId"]))
        out("")
        out("ASSEGNAZIONI")
        if p["assignments"]:
            _table(
                [
                    [
                        a["key"],
                        a["workspace"],
                        a["mode"],
                        ", ".join(a["resources"]) or "-",
                    ]
                    for a in p["assignments"]
                ],
                ["ITEM", "WORKSPACE", "MODE", "RES"],
            )
            for a in p["assignments"]:
                if a["mode"] == "TEMPORARY_WORKTREE_SUGGESTED":
                    out("  ! {}: {}".format(a["key"], a["reason"]))
        else:
            out("  nessuna.")
        out("")
        out("RIMANDATE")
        if p["deferred"]:
            _table(
                [
                    [d["key"], _dash(d["workspace"]), d["reason"], d["detail"]]
                    for d in p["deferred"]
                ],
                ["ITEM", "WORKSPACE", "REASON", "DETTAGLIO"],
            )
        else:
            out("  nessuna.")
        out("")
        out("CAPACITA'")
        rows = [
            [
                "writer:{}".format(g),
                "{}/{}".format(v["used"], v["capacity"]),
            ]
            for g, v in p["capacity"]["writers"].items()
        ]
        rows.append(
            [
                "temporaryWorktrees",
                "{}/{}".format(
                    p["capacity"]["temporaryWorktrees"]["used"],
                    p["capacity"]["temporaryWorktrees"]["capacity"],
                ),
            ]
        )
        rows.append(
            [
                "unrealEditor",
                "{}/{}".format(
                    p["capacity"]["unrealEditor"]["used"],
                    p["capacity"]["unrealEditor"]["capacity"],
                ),
            ]
        )
        rows.append(["wip globale", str(p["wip"]["global"])])
        _table(rows, ["RISORSA", "USO"])

    emit(args, payload, render)
    return 0


def cmd_roadmap_state_set(args):
    client = _client(args)
    session_id, _ = _session_id(args, required=False)
    row = client.call(
        "roadmap.setState",
        roadmapId=args.id,
        itemKey=args.item,
        progress=args.progress,
        sessionId=session_id,
        candidateId=args.candidate,
        note=args.note,
        mode=args.mode,
    )
    emit(
        args,
        row,
        lambda r: out(
            "{} -> {}{}{}".format(
                r["item_key"],
                r["progress"],
                " su {}".format(r["mode"]) if r.get("mode") else "",
                " (candidate {})".format(r["candidate_id"])
                if r.get("candidate_id")
                else "",
            )
        ),
    )
    return 0


def cmd_roadmap_state_list(args):
    client = _client(args)
    payload = client.call("roadmap.states", roadmapId=args.id)

    def render(p):
        if not p["states"]:
            out("nessuno stato dichiarato: tutte le issue sono PENDING.")
            return
        _table(
            [
                [
                    s["item_key"],
                    s["progress"],
                    _dash(s.get("mode")),
                    _dash(s.get("candidate_id")),
                    s["updated_at"],
                    _dash(s.get("updated_by")),
                ]
                for s in p["states"]
            ],
            ["ITEM", "PROGRESS", "MODE", "CANDIDATE", "UPDATED", "BY"],
        )

    emit(args, payload, render)
    return 0


# ---------------------------------------------------------------------------
# status generale
# ---------------------------------------------------------------------------


def cmd_status(args):
    from .binding import resolve
    from .daemon import probe, read_daemon_file
    from .paths import db_path, store_root

    health = probe()
    info = read_daemon_file()
    session_id, origin = resolve(getattr(args, "session", None), required=False)

    payload = {
        "coordinator": {
            "running": health is not None,
            "endpoint": "{}:{}".format(info["host"], info["port"]) if info else None,
            "pid": (health or {}).get("pid"),
            "protocolVersion": PROTOCOL_VERSION,
            "schemaVersion": SCHEMA_VERSION,
            "daemonProtocolVersion": (info or {}).get("protocolVersion"),
            "dbSchemaVersion": (health or {}).get("dbSchemaVersion"),
        },
        "store": {"root": store_root(), "db": db_path()},
        "session": None,
        "sessionOrigin": origin,
        "pending": None,
        "git": collect_git(os.getcwd()),
    }

    if health is not None and session_id:
        client = _client(args)
        try:
            payload["session"] = client.call("session.get", sessionId=session_id)
            payload["pending"] = client.call("inbox.count", sessionId=session_id)["pending"]
        except Rt3Error as exc:
            payload["sessionError"] = str(exc)

    def render(p):
        c = p["coordinator"]
        out("RT3 control plane")
        out(
            "  coordinator    : {}".format(
                "ATTIVO su {} (pid {})".format(c["endpoint"], c["pid"])
                if c["running"]
                else "NON attivo  -  `rt3 daemon start`"
            )
        )
        out(
            "  versioni       : client protocollo v{} / schema v{}{}".format(
                c["protocolVersion"],
                c["schemaVersion"],
                (
                    "  |  daemon protocollo v{} / db schema v{}".format(
                        c["daemonProtocolVersion"], c["dbSchemaVersion"]
                    )
                    if c["running"]
                    else ""
                ),
            )
        )
        out("  store          : {}".format(p["store"]["db"]))
        out("")
        s = p["session"]
        if s is None:
            out(
                "  sessione       : {}".format(
                    p.get("sessionError")
                    or "nessuna  -  `rt3 session start --id ... --role ... --lane ...`"
                )
            )
        else:
            out("  sessione       : {} [{}]".format(s["session_id"], s["status"]))
            out("  origine id     : {}".format(p["sessionOrigin"]))
            out("  role / lane    : {} / {}".format(s["role"], s["lane"]))
            out("  workspaceGroup : {}".format(s["workspace_group"]))
            out("  task           : {}".format(_dash(s["task_id"])))
            out("  worktree       : {}".format(_dash(s["worktree_path"])))
            out(
                "  branch @ HEAD  : {} @ {}".format(
                    _branch(s["branch"]), short_head(s["head"])
                )
            )
            out("  writeMode      : {}".format(s["write_mode"]))
            out("  eventi pending : {}".format(p["pending"]))
        g = p["git"]
        out("")
        out("  git qui        : {} @ {}".format(_branch(g["branch"]), short_head(g["head"])))
        out("  worktree qui   : {}".format(_dash(g["worktreePath"])))

    emit(args, payload, render)
    return 0


def cmd_version(args):
    """Le tre versioni di questo CHECKOUT, senza chiedere nulla al daemon.

    E' il comando che risponde alla domanda «i tre workspace sono allineati?»: girandolo
    nei tre checkout si confrontano tre righe. Interrogare il daemon direbbe la versione
    del daemon - una sola, la stessa per tutti - che e' un'altra domanda.
    """
    payload = {
        "protocolVersion": PROTOCOL_VERSION,
        "schemaVersion": SCHEMA_VERSION,
        "roadmapSchemaVersion": ROADMAP_SCHEMA_VERSION,
        "python": sys.version.split()[0],
    }
    emit(
        args,
        payload,
        lambda p: out(
            "rt3 protocollo v{} | schema v{} | roadmap v{} | python {}".format(
                p["protocolVersion"],
                p["schemaVersion"],
                p["roadmapSchemaVersion"],
                p["python"],
            )
        ),
    )
    return 0


# ---------------------------------------------------------------------------
# Parser
# ---------------------------------------------------------------------------


def build_parser():
    parser = argparse.ArgumentParser(
        prog="rt3",
        description="RT3 control plane: sessioni, eventi e mailbox fra i terminali RT3.",
    )
    parser.add_argument("--json", action="store_true", help="output JSON")
    parser.add_argument(
        "--session", help="SessionId esplicito (ha precedenza sul binding del terminale)"
    )
    parser.add_argument("--timeout", type=float, default=30.0, help="timeout HTTP (s)")
    sub = parser.add_subparsers(dest="command")

    # -- daemon
    d = sub.add_parser("daemon", help="coordinator locale rt3d").add_subparsers(
        dest="sub"
    )
    p = d.add_parser("start", help="avvia rt3d in background")
    p.set_defaults(func=cmd_daemon_start)
    p = d.add_parser("run", help="esegue rt3d in primo piano (diagnosi)")
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=0)
    p.set_defaults(func=cmd_daemon_run)
    p = d.add_parser("status", help="stato di rt3d")
    p.set_defaults(func=cmd_daemon_status)
    p = d.add_parser("stop", help="ferma rt3d")
    p.set_defaults(func=cmd_daemon_stop)
    p = d.add_parser("restart", help="ferma e riavvia rt3d")
    p.set_defaults(func=cmd_daemon_restart)

    # -- session
    s = sub.add_parser("session", help="sessione di questo terminale").add_subparsers(
        dest="sub"
    )
    p = s.add_parser("start", help="registra la sessione e lega il terminale")
    p.add_argument("--id", required=True, help="SessionId, es. DEV-1")
    p.add_argument("--role", required=True, choices=ROLES)
    p.add_argument("--lane", required=True, choices=LANES)
    p.add_argument("--workspace-group", required=True, choices=WORKSPACE_GROUPS)
    p.add_argument("--task", help="TaskId su cui la sessione lavora")
    p.add_argument("--write-mode", default="READ_ONLY", choices=WRITE_MODES)
    p.add_argument("--write-set", nargs="*", help="path dichiarati in scrittura")
    p.add_argument("--worktree", help="directory da ispezionare (default: cwd)")
    p.add_argument(
        "--replace",
        action="store_true",
        help="riusa un SessionId ancora ATTIVO (terminale morto senza cleanup)",
    )
    p.set_defaults(func=cmd_session_start)
    p = s.add_parser("status", help="stato della sessione corrente")
    p.set_defaults(func=cmd_session_status)
    p = s.add_parser("set", help="aggiorna task, writeMode o metadati Git")
    p.add_argument("--task")
    p.add_argument("--write-mode", choices=WRITE_MODES)
    p.add_argument("--unreal-lease", choices=("NONE", "REQUESTED", "OWNED"))
    p.add_argument(
        "--refresh-git", action="store_true", help="rilegge branch e HEAD dalla cwd"
    )
    p.set_defaults(func=cmd_session_set)
    p = s.add_parser("stop", help="ferma la sessione e scioglie il binding")
    p.set_defaults(func=cmd_session_stop)

    p = sub.add_parser("sessions", help="elenco sessioni").add_subparsers(dest="sub")
    q = p.add_parser("list")
    q.add_argument("--all", action="store_true", help="include le sessioni fermate")
    q.set_defaults(func=cmd_sessions_list)

    # -- event
    e = sub.add_parser("event", help="pubblicazione eventi").add_subparsers(dest="sub")
    p = e.add_parser("publish", help="pubblica un evento")
    p.add_argument("--type", required=True, choices=EVENT_TYPES)
    p.add_argument("--task")
    p.add_argument("--candidate")
    p.add_argument("--note")
    p.add_argument("--payload", help="oggetto JSON")
    p.add_argument("--to-session", help="destinatario nominato (sessione)")
    p.add_argument("--to-role", choices=ROLES, help="destinatario nominato (ruolo)")
    p.add_argument("--to-lane", choices=LANES, help="lane del destinatario nominato")
    p.add_argument(
        "--with-git",
        action="store_true",
        help="aggiunge branch, HEAD e worktree al payload",
    )
    p.set_defaults(func=cmd_event_publish)

    p = sub.add_parser("events", help="event log").add_subparsers(dest="sub")
    q = p.add_parser("list")
    q.add_argument("--limit", type=int, default=30)
    q.add_argument("--task")
    q.set_defaults(func=cmd_events_list)

    # -- inbox
    i = sub.add_parser("inbox", help="mailbox della sessione").add_subparsers(dest="sub")
    p = i.add_parser("list", help="eventi pendenti per me")
    p.add_argument("--all", action="store_true", help="include quelli gia' presi")
    p.add_argument("--limit", type=int, default=50)
    p.set_defaults(func=cmd_inbox_list)
    p = i.add_parser("show", help="dettaglio di un evento")
    p.add_argument("ref", help="event-id oppure delivery-id")
    p.set_defaults(func=cmd_inbox_show)
    p = i.add_parser("ack", help="prende in carico un evento")
    p.add_argument("ref", help="event-id oppure delivery-id")
    p.add_argument("--note")
    p.set_defaults(func=cmd_inbox_ack)

    # -- task
    t = sub.add_parser("task", help="task registry").add_subparsers(dest="sub")
    p = t.add_parser("show")
    p.add_argument("task_id")
    p.set_defaults(func=cmd_task_show)

    p = sub.add_parser("tasks", help="elenco task").add_subparsers(dest="sub")
    q = p.add_parser("list")
    q.set_defaults(func=cmd_tasks_list)

    # -- candidate
    c = sub.add_parser("candidate", help="metadati candidate").add_subparsers(dest="sub")
    p = c.add_parser("create")
    p.add_argument("--task")
    p.add_argument("--branch")
    p.add_argument("--head")
    p.add_argument("--note")
    p.add_argument("--roadmap", dest="roadmap_id", help="roadmap a cui appartiene")
    p.add_argument("--item", dest="item_key", help="issue della roadmap, es. EPIC-B/B2")
    p.set_defaults(func=cmd_candidate_create)
    p = c.add_parser("status", help="registra l'esito della validazione")
    p.add_argument("candidate_id")
    p.add_argument("--status", required=True, choices=CANDIDATE_STATUSES)
    p.add_argument("--note")
    p.set_defaults(func=cmd_candidate_status)

    p = sub.add_parser("candidates", help="elenco candidate").add_subparsers(dest="sub")
    q = p.add_parser("list")
    q.add_argument("--task")
    q.set_defaults(func=cmd_candidates_list)

    # -- roadmap
    #
    # L'elenco sta nel PLURALE top-level, come `sessions`, `tasks`, `events` e
    # `candidates`: una CLI in cui un elenco su cinque sta altrove costringe a
    # ricordare l'eccezione invece della regola.
    p = sub.add_parser("roadmaps", help="roadmap caricate").add_subparsers(dest="sub")
    q = p.add_parser("list")
    q.set_defaults(func=cmd_roadmaps_list)

    r = sub.add_parser(
        "roadmap", help="roadmap orchestration: PLAN, readiness, piano"
    ).add_subparsers(dest="sub")

    p = r.add_parser("validate", help="valida un file roadmap (non richiede rt3d)")
    p.add_argument("--file", required=True)
    p.set_defaults(func=cmd_roadmap_validate)

    p = r.add_parser("load", help="valida e carica una roadmap nel control plane")
    p.add_argument("--file", required=True)
    p.add_argument(
        "--reset-state",
        action="store_true",
        help="azzera lo stato runtime della roadmap (distruttivo: NON e' il default)",
    )
    p.set_defaults(func=cmd_roadmap_load)

    p = r.add_parser("show", help="quadro di una roadmap")
    p.add_argument("--id")
    p.set_defaults(func=cmd_roadmap_show)

    p = r.add_parser("graph", help="grafo delle dipendenze")
    p.add_argument("--id")
    p.set_defaults(func=cmd_roadmap_graph)

    p = r.add_parser("critical-path", help="cammino critico e slack")
    p.add_argument("--id")
    p.set_defaults(func=cmd_roadmap_critical_path)

    p = r.add_parser("ready", help="cosa e' READY e cosa e' BLOCKED, e perche'")
    p.add_argument("--id")
    p.add_argument(
        "--state",
        choices=("READY", "BLOCKED", "IN_PROGRESS", "VALIDATED", "DONE"),
        help="mostra solo questo stato",
    )
    p.set_defaults(func=cmd_roadmap_ready)

    p = r.add_parser("plan", help="assegnazioni e rimandi, con le capacita'")
    p.add_argument("--id")
    p.set_defaults(func=cmd_roadmap_plan)

    st = r.add_parser("state", help="stato runtime delle issue").add_subparsers(
        dest="sub2"
    )
    p = st.add_parser("set", help="avanza una issue")
    p.add_argument("item", help="chiave EPIC/ISSUE, o la forma breve se non ambigua")
    p.add_argument("--progress", required=True, choices=ITEM_PROGRESS_STATES)
    p.add_argument("--id", help="roadmap, se ne e' caricata piu' di una")
    p.add_argument("--candidate", help="candidate che ha prodotto questo stato")
    p.add_argument(
        "--mode",
        choices=ITEM_MODES,
        help="dove la issue viene lavorata. Omesso = PERMANENT_WRITER, che e' la "
        "lettura conservativa: chi lavora su un worktree temporaneo DEVE dichiararlo, "
        "altrimenti il piano conta un writer permanente che non e' occupato",
    )
    p.add_argument("--note")
    p.set_defaults(func=cmd_roadmap_state_set)
    p = st.add_parser("list", help="stati dichiarati")
    p.add_argument("--id")
    p.set_defaults(func=cmd_roadmap_state_list)

    p = sub.add_parser("status", help="quadro sintetico")
    p.set_defaults(func=cmd_status)

    p = sub.add_parser("version", help="versioni di protocollo e schema")
    p.set_defaults(func=cmd_version)

    return parser


def main(argv=None):
    parser = build_parser()
    args = parser.parse_args(argv)
    func = getattr(args, "func", None)
    if func is None:
        parser.print_help()
        return 2
    try:
        return func(args) or 0
    except Rt3Error as exc:
        sys.stderr.write(str(exc) + "\n")
        return exc.exit_code
    except KeyboardInterrupt:  # pragma: no cover
        sys.stderr.write("interrotto.\n")
        return 130
