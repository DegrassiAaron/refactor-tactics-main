"""Client HTTP verso `rt3d`.

La CLI non tocca mai SQLite: passa di qui. E' la separazione che rende vera la frase
"tutte le sessioni comunicano con rt3d" - se il client sapesse aprire il database, la
prima volta che il daemon non risponde qualcuno aggiungerebbe un ripiego, e da quel
momento due processi scriverebbero lo stesso file senza conoscersi.
"""

import json
import urllib.error
import urllib.request

from . import PROTOCOL_VERSION
from .daemon import PROTOCOL_HEADER, read_daemon_file
from .errors import (
    DaemonUnavailable,
    ProtocolMismatch,
    Rt3Error,
)

#: Mappa codice -> classe, per ricostruire in locale l'errore che il daemon ha rifiutato.
#: Senza, ogni rifiuto arriverebbe come Rt3Error generico e il chiamante perderebbe la
#: possibilita' di distinguere un conflitto di ack da un evento inesistente.
_ERROR_CODES = {}


def _register_errors():
    from . import errors as _errors

    for name in dir(_errors):
        obj = getattr(_errors, name)
        if isinstance(obj, type) and issubclass(obj, Rt3Error):
            _ERROR_CODES[obj.code] = obj


_register_errors()


class Client:
    def __init__(self, host, port, timeout=30.0):
        self.host = host
        self.port = port
        self.timeout = timeout

    @property
    def base_url(self):
        return "http://{}:{}".format(self.host, self.port)

    def _request(self, path, payload=None, method="POST"):
        url = self.base_url + path
        data = json.dumps(payload).encode("utf-8") if payload is not None else None
        req = urllib.request.Request(url, data=data, method=method)
        req.add_header("Content-Type", "application/json; charset=utf-8")
        req.add_header(PROTOCOL_HEADER, str(PROTOCOL_VERSION))
        try:
            with urllib.request.urlopen(req, timeout=self.timeout) as resp:
                return json.loads(resp.read().decode("utf-8"))
        except urllib.error.HTTPError as exc:
            try:
                body = json.loads(exc.read().decode("utf-8"))
            except Exception:
                raise Rt3Error(
                    "rt3d ha risposto {} senza corpo interpretabile.".format(exc.code)
                )
            code = body.get("code", "RT3_ERROR")
            message = body.get("error", "errore non descritto")
            if code == "RT3_PROTOCOL_MISMATCH":
                raise ProtocolMismatch(message)
            raise _ERROR_CODES.get(code, Rt3Error)(message, code=code)
        except urllib.error.URLError as exc:
            raise DaemonUnavailable(
                "rt3d non raggiungibile su {} ({}). Avviarlo con `rt3 daemon "
                "start`.".format(self.base_url, exc.reason)
            )
        except OSError as exc:
            raise DaemonUnavailable(
                "rt3d non raggiungibile su {} ({}).".format(self.base_url, exc)
            )

    def health(self):
        body = self._request("/health", method="GET")
        return body.get("result", body)

    def call(self, op, **args):
        body = self._request("/rpc", {"op": op, "args": args})
        return body.get("result")


def connect(timeout=30.0):
    """Client verso il daemon pubblicato in `daemon.json`.

    Solleva `DaemonUnavailable` quando il file non c'e' - lo stato normale prima del
    primo `rt3 daemon start`, che va detto come istruzione e non come guasto.
    """
    info = read_daemon_file()
    if info is None:
        raise DaemonUnavailable(
            "nessun rt3d registrato su questa macchina. Avviarlo con "
            "`rt3 daemon start`."
        )
    client = Client(info["host"], info["port"], timeout=timeout)

    declared = info.get("protocolVersion")
    if declared is not None and declared != PROTOCOL_VERSION:
        raise ProtocolMismatch(
            "rt3d in esecuzione parla protocollo RT3 v{}, questo client parla v{}. "
            "I tre workspace devono eseguire la stessa versione del control plane: "
            "allineare i checkout, poi `rt3 daemon restart`.".format(
                declared, PROTOCOL_VERSION
            )
        )
    return client
