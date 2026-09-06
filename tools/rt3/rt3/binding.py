"""Quale sessione RT3 rappresenta QUESTO terminale.

La specifica (§12) chiede esplicitamente di non dedurlo dalla current working directory,
e ha ragione: due terminali aperti nello stesso checkout sono due sessioni diverse - DEV
e VALIDATION possono benissimo lavorare dalla stessa directory - e la cwd non li
distingue.

Precedenza, dalla piu' forte alla piu' debole:

    1. `--session <id>`      argomento esplicito, per script e diagnosi
    2. `RT3_SESSION_ID`      variabile d'ambiente, il binding piu' robusto
    3. binding per console   file scritto da `rt3 session start`, indicizzato dal PID
                             del processo che ha invocato la CLI (la shell)
    4. errore                RT3_SESSION_UNBOUND, con l'istruzione per uscirne

Il livello 3 esiste perche' il livello 2 ha un difetto pratico: un processo figlio non
puo' scrivere una variabile d'ambiente nel padre, quindi `rt3 session start` non puo'
esportare `RT3_SESSION_ID` nella shell che lo ha lanciato. Puo' pero' scrivere un file
indicizzato dal PID di quella shell, che vive quanto il terminale e cambia a ogni
terminale nuovo. E' la stessa idea del terminal-scoped state, ottenuta senza chiedere
all'utente di ricordarsi un export.

⚠️ Limite noto e accettato: i PID vengono riusati dal sistema operativo. Una shell nuova
che ricevesse il PID di una vecchia erediterebbe il suo binding. Il caso e' raro (il
file viene rimosso da `session stop`) e non silenzioso: `rt3 status` stampa sempre la
sessione risolta e da dove viene, quindi il primo comando mostra l'anomalia. La via
d'uscita e' `RT3_SESSION_ID`, che ha precedenza.
"""

import json
import os

from .errors import SessionUnbound
from .model import now_iso
from .paths import bindings_dir

ENV_SESSION = "RT3_SESSION_ID"


def _binding_path(pid):
    return os.path.join(bindings_dir(), "console-{}.json".format(pid))


def console_key():
    """PID della shell che ha invocato la CLI.

    E' `os.getppid()`: la catena e' `pwsh -> python`, perche' `rt3.ps1` e' uno script e
    non crea un processo intermedio. Lanciando `python -m rt3` da bash il padre e' bash,
    che va altrettanto bene: cio' che serve e' un identificatore stabile per la vita del
    terminale e diverso per ogni terminale.
    """
    try:
        return os.getppid()
    except OSError:  # pragma: no cover - piattaforme esotiche
        return os.getpid()


def bind(session_id, pid=None):
    """Lega il terminale corrente a una sessione. Ritorna il path scritto."""
    pid = pid if pid is not None else console_key()
    directory = bindings_dir()
    os.makedirs(directory, exist_ok=True)
    path = _binding_path(pid)
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as fh:
        json.dump(
            {"sessionId": session_id, "pid": pid, "boundAt": now_iso()}, fh, indent=2
        )
    os.replace(tmp, path)
    return path


def unbind(pid=None):
    pid = pid if pid is not None else console_key()
    try:
        os.remove(_binding_path(pid))
        return True
    except OSError:
        return False


def read_binding(pid=None):
    pid = pid if pid is not None else console_key()
    try:
        with open(_binding_path(pid), "r", encoding="utf-8") as fh:
            return json.load(fh)
    except (OSError, ValueError):
        return None


def resolve(explicit=None, required=True):
    """Ritorna `(session_id, origine)`.

    L'origine viaggia insieme all'id perche' un binding sbagliato e un id sbagliato si
    correggono in modi diversi, e senza l'origine non si distinguono.
    """
    if explicit:
        return explicit, "--session"

    from_env = os.environ.get(ENV_SESSION)
    if from_env and from_env.strip():
        return from_env.strip(), ENV_SESSION

    binding = read_binding()
    if binding and binding.get("sessionId"):
        return binding["sessionId"], "binding console pid={}".format(binding.get("pid"))

    if not required:
        return None, None

    raise SessionUnbound(
        "questo terminale non e' legato ad alcuna sessione RT3. Registrarne una con "
        "`rt3 session start --id <SessionId> --role <DEV|EDITOR|VALIDATION> "
        "--lane <MAIN|DEV|DESIGNER> --workspace-group <MAIN|DEV|DESIGNER>`, oppure "
        "indicarla con --session <id> o con la variabile {}.".format(ENV_SESSION)
    )
