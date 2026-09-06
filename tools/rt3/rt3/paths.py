"""Dove vive lo stato runtime di RT3.

🔴 Lo store e' PER MACCHINA, non per repository, e questa e' la decisione piu'
importante del pacchetto.

La specifica proponeva `.rt3/runtime.db` dentro il checkout. Non funziona qui: i tre
workspace permanenti - `refactor-tactics-main`, `refactor-tactict-dev`,
`refactor-tactics-technical-designer` - sono tre CLONI distinti dello stesso remote,
ciascuno col proprio `.git`. Un database dentro il checkout ne produrrebbe TRE, e tre
control plane che non si vedono sono esattamente cio' che il control plane esiste per
evitare: DEV pubblicherebbe `TASK_READY` nel proprio database e l'EDITOR aprirebbe il
proprio, vuoto, concludendo che non c'e' lavoro.

La radice e' la stessa gia' usata dagli altri strumenti RT3 della macchina -
`rt-workspace.ps1` per il registro dei workspace, `rt-task-router.ps1` per i task - e
non e' un caso: sono tutti stato per macchina, per la stessa ragione.

    %LOCALAPPDATA%\\RefactorTactics\\RT3\\        (Windows)
    ~/.local/share/RefactorTactics/RT3/         (POSIX, o $XDG_DATA_HOME)

`RT3_HOME` sovrascrive la radice. Serve ai test - che devono girare su uno store usa e
getta senza toccare quello vero - e alla diagnosi. In esercizio non va passata: puntarla
dentro un checkout ricrea il difetto descritto qui sopra.

⛔ Nulla di quanto vive qui va versionato: e' stato runtime di UNA macchina. Il
`.gitignore` porta comunque una riga `.rt3/` come rete di sicurezza, per il caso in cui
qualcuno punti `RT3_HOME` dentro il repository.
"""

import os

from .errors import StoreUnavailable

ENV_HOME = "RT3_HOME"


def store_root():
    """Radice dello store RT3 per questa macchina. Non crea nulla."""
    override = os.environ.get(ENV_HOME)
    if override and override.strip():
        return os.path.abspath(os.path.expanduser(override.strip()))

    if os.name == "nt":
        base = os.environ.get("LOCALAPPDATA")
        if not base:
            raise StoreUnavailable(
                "LOCALAPPDATA non definito: la radice RT3 per macchina non e' "
                "localizzabile. Definire LOCALAPPDATA oppure RT3_HOME."
            )
    else:
        base = os.environ.get("XDG_DATA_HOME")
        if not base:
            home = os.environ.get("HOME")
            if not home:
                raise StoreUnavailable(
                    "HOME non definito: la radice RT3 per macchina non e' "
                    "localizzabile. Definire HOME oppure RT3_HOME."
                )
            base = os.path.join(home, ".local", "share")

    return os.path.join(os.path.abspath(base), "RefactorTactics", "RT3")


def ensure_store_root():
    """Radice dello store, creata se manca."""
    root = store_root()
    try:
        os.makedirs(root, exist_ok=True)
    except OSError as exc:
        raise StoreUnavailable(
            "impossibile creare la radice RT3 {}: {}".format(root, exc)
        )
    return root


def db_path():
    return os.path.join(store_root(), "runtime.db")


def daemon_file():
    """Endpoint pubblicato da `rt3d`: host, porta, pid, versioni.

    Il daemon ascolta su una porta EFFIMERA e la scrive qui. Una porta fissa
    sembrerebbe piu' semplice, ma su una workstation con tre VS Code, un Unreal e un
    ponte MCP gia' in ascolto, una porta fissa e' una collisione che si manifesta come
    "il daemon non parte" senza dire perche'.
    """
    return os.path.join(store_root(), "daemon.json")


def bindings_dir():
    return os.path.join(store_root(), "bindings")


def daemon_log():
    return os.path.join(store_root(), "rt3d.log")
