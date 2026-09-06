"""Impalcatura comune ai test RT3.

Due proprieta' che i test devono avere, e che si ottengono solo qui:

    isolamento   ogni test gira su un `RT3_HOME` usa e getta. Senza, la prima
                 esecuzione scriverebbe nel control plane VERO della macchina, e la
                 seconda troverebbe le sessioni della prima.

    realta'      il daemon di prova e' il daemon vero (`Rt3Server`), raggiunto via
                 HTTP dal client vero. Un finto store in memoria proverebbe le regole
                 e non il trasporto - e il trasporto e' meta' di cio' che questa
                 milestone consegna.
"""

import os
import shutil
import tempfile
import threading
import unittest


class Rt3TestCase(unittest.TestCase):
    """Base con `RT3_HOME` isolato per ogni test."""

    def setUp(self):
        self._prev_home = os.environ.get("RT3_HOME")
        self._prev_session = os.environ.get("RT3_SESSION_ID")
        self.home = tempfile.mkdtemp(prefix="rt3-test-")
        os.environ["RT3_HOME"] = self.home
        os.environ.pop("RT3_SESSION_ID", None)
        self._stores = []

    def tearDown(self):
        # Chiudere le connessioni prima di cancellare la directory: senza, Python
        # emette ResourceWarning e su Windows il file resta aperto mentre rmtree prova
        # a toglierlo.
        for store in self._stores:
            try:
                store.close()
            except Exception:
                pass
        if self._prev_home is None:
            os.environ.pop("RT3_HOME", None)
        else:
            os.environ["RT3_HOME"] = self._prev_home
        if self._prev_session is not None:
            os.environ["RT3_SESSION_ID"] = self._prev_session
        shutil.rmtree(self.home, ignore_errors=True)

    def open_store(self):
        from rt3.paths import db_path, ensure_store_root
        from rt3.store import open_store

        ensure_store_root()
        store = open_store(db_path())
        self._stores.append(store)
        return store


class LocalDaemon:
    """`rt3d` vero, in-process, su porta effimera.

    Serve a provare il ciclo completo CLI -> HTTP -> store, e soprattutto a provare il
    RESTART: fermare questo e avviarne un altro sullo stesso `RT3_HOME` e' esattamente
    cio' che accade quando il terminale che ospitava rt3d viene chiuso.
    """

    def __init__(self):
        from rt3.daemon import Rt3Server, write_daemon_file
        from rt3.paths import db_path, ensure_store_root
        from rt3.store import open_store

        ensure_store_root()
        self.store = open_store(db_path())
        self.server = Rt3Server(("127.0.0.1", 0), self.store)
        host, port = self.server.server_address[0], self.server.server_address[1]
        self.info = write_daemon_file(host, port)
        self.thread = threading.Thread(
            target=self.server.serve_forever, kwargs={"poll_interval": 0.05}, daemon=True
        )
        self.thread.start()

    def stop(self):
        from rt3.daemon import clear_daemon_file

        self.server.shutdown()
        self.thread.join(timeout=5)
        self.server.server_close()
        self.store.close()
        clear_daemon_file(expected_pid=os.getpid())

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.stop()
        return False


def client():
    from rt3.client import connect

    return connect(timeout=10.0)


def smoke_roadmap_path():
    """Path della roadmap di smoke versionata.

    Risolto da `__file__` e non dalla cwd: i test si lanciano indifferentemente dalla
    radice del repository o da `tools/rt3`, e un path relativo alla cwd fallirebbe in
    uno dei due casi - quello che gira sull'altra macchina.
    """
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.abspath(os.path.join(here, "..", "..", ".."))
    return os.path.join(
        repo, "docs", "rt-three-terminals", "roadmaps", "rt3-smoke-multi-epic.yaml"
    )


def load_smoke_roadmap():
    """`(roadmap, graph)` della roadmap di smoke, gia' verificata."""
    from rt3.graph import load_checked

    roadmap, graph, _problems = load_checked(smoke_roadmap_path())
    return roadmap, graph


def start_session(cli, session_id, role, lane, workspace_group=None, **extra):
    """Registra una sessione con i soli campi che il test si cura di dichiarare."""
    return cli.call(
        "session.start",
        sessionId=session_id,
        role=role,
        lane=lane,
        workspaceGroup=workspace_group or lane,
        **extra,
    )
