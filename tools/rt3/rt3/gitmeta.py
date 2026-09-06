"""Metadati Git di un terminale: repoRoot, worktreePath, branch, HEAD.

Il control plane li REGISTRA, non li governa. Nessuna funzione qui muta il repository, e
questa milestone non blocca alcuna operazione Git: serve che una sessione EDITOR possa
leggere da dove arriva il lavoro di DEV senza chiederlo in chat.

⚠️ Due casi che sembrano guasti e non lo sono:

    detached HEAD       `branch` vale None. E' lo stato normale di `D:/rt-build-main`,
                        e di ogni worktree creato con `--detach`. Chi legge il campo
                        deve accettare None; chi lo stampa scrive `(detached)`.

    fuori da un repo    tutti i campi valgono None. Una sessione RT3 puo' legittimamente
                        essere aperta in una directory qualunque - la registrazione non
                        deve fallire per questo.

⛔ `worktreePath` NON e' `repoRoot` quando la sessione lavora in un `git worktree`
separato: sono due colonne diverse apposta. E nessuno dei due si deduce dal nome della
finestra VS Code, che e' la regola di AGENTS.md §11 - una cartella chiamata `Dev` e' un
luogo, non una figura.
"""

import os
import subprocess


def _git(args, cwd):
    """Esegue git e ritorna stdout ripulito, oppure None se il comando fallisce.

    Non solleva: l'assenza di Git, o una directory che non e' un repository, sono esiti
    previsti (vedi il docstring del modulo) e non devono impedire la registrazione di
    una sessione.
    """
    try:
        proc = subprocess.run(
            ["git"] + args,
            cwd=cwd,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            timeout=15,
        )
    except (OSError, subprocess.SubprocessError):
        return None
    if proc.returncode != 0:
        return None
    return proc.stdout.decode("utf-8", "replace").strip() or None


def collect(cwd=None):
    """Metadati Git della directory indicata (default: quella corrente).

    Ritorna sempre un dizionario con le quattro chiavi. Un valore None significa "non
    determinabile", mai "non ancora letto".
    """
    cwd = os.path.abspath(cwd or os.getcwd())

    meta = {
        "repoRoot": None,
        "worktreePath": None,
        "branch": None,
        "head": None,
    }

    if not os.path.isdir(cwd):
        return meta

    # `--show-toplevel` da' la radice del WORKTREE corrente, che in un worktree separato
    # non e' la radice del clone. E' il valore che serve a chi deve sapere dove stanno i
    # file di questa sessione.
    worktree = _git(["rev-parse", "--show-toplevel"], cwd)
    if worktree is None:
        return meta
    meta["worktreePath"] = os.path.normpath(worktree)

    # La radice del CLONE si ricava dal git-common-dir: in un worktree separato punta al
    # `.git` del clone principale, ed e' cosi' che due worktree si riconoscono parenti.
    common = _git(["rev-parse", "--path-format=absolute", "--git-common-dir"], cwd)
    if common:
        common = os.path.normpath(common)
        # `<clone>/.git` -> `<clone>`; per un repo bare o inatteso si tiene com'e'.
        if os.path.basename(common) == ".git":
            meta["repoRoot"] = os.path.dirname(common)
        else:
            meta["repoRoot"] = common
    else:
        meta["repoRoot"] = meta["worktreePath"]

    # `--symbolic-full-name HEAD` da' `refs/heads/<branch>` su un branch e "HEAD" in
    # detached. `--abbrev-ref` restituisce la stringa "HEAD" in detached, che si
    # confonde con un branch chiamato HEAD: la forma piena non ha quell'ambiguita'.
    symbolic = _git(["rev-parse", "--symbolic-full-name", "HEAD"], cwd)
    if symbolic and symbolic.startswith("refs/heads/"):
        meta["branch"] = symbolic[len("refs/heads/") :]
    else:
        meta["branch"] = None  # detached HEAD, o repository senza commit

    meta["head"] = _git(["rev-parse", "HEAD"], cwd)

    return meta


def short_head(head):
    """Forma abbreviata a 8 caratteri, o `-` se lo sha non c'e'."""
    if not head:
        return "-"
    return head[:8]
