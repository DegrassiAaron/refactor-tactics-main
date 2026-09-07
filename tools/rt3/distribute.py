"""Propagare una modifica fra i workspace RT3, scegliendo la strada dai fatti.

⛔ Sta ACCANTO al pacchetto `rt3/` e non dentro, e la ragione e' un vincolo dichiarato:
«nessuna funzione [di `rt3/`] esegue un comando che muta il repository. Git e' letto, mai
scritto». Questo modulo Git lo scrive. Tenerlo fuori mantiene vera quella frase.

## La catena

    stesso repo / worktree        ->  nessun trasporto: gli oggetti sono gia' condivisi
    cloni distinti, stesso remote ->  fetch da PATH LOCALE, poi materializzazione
    Git non utilizzabile          ->  copia controllata, come ultima risorsa

Il punto della prima riga e' che **non c'e' niente da trasportare**. Se due directory
sono worktree dello stesso repository, un commit fatto in una e' gia' visibile all'altra:
mandarlo su GitHub per riscaricarlo e' un giro inutile, e il remoto serve
all'INTEGRAZIONE (PR, review, main), non al trasporto.

⚠️ Il caso si MISURA. `--git-common-dir` uguale significa stesso repository; diverso
significa cloni distinti. Non si deduce dal nome della directory: su questa macchina
`refactor-tactics-technical-designer` **contiene** un clone senza esserlo.

## Cosa NON fa

Non fa merge, non fa checkout, non sposta `HEAD`, non tocca l'indice. Porta gli oggetti e
materializza i file; integrare e' un gesto separato, di chi possiede il ramo. In un
working tree condiviso spostare `HEAD` significa spostarlo sotto un'altra sessione.

Uso:
    python tools/rt3/distribute.py --to <path> --commit <sha> [--paths p1 p2 ...]
    python tools/rt3/distribute.py --to <path> --commit <sha> --dry-run
"""

import argparse
import os
import shutil
import subprocess
import sys

#: Cosa una copia controllata non porta MAI. Non e' un'ottimizzazione: `.git` copiato
#: sovrascrive la storia del bersaglio, e `runtime.db` e' lo stato di UNA macchina - due
#: modi diversi di distruggere qualcosa che non ci appartiene.
COPY_DENY = (
    ".git",
    "Binaries",
    "Intermediate",
    "Saved",
    "DerivedDataCache",
    "__pycache__",
    ".rt3",
    "runtime.db",
    "daemon.json",
    "bindings",
)

#: Esiti della classificazione.
SAME_REPO = "SAME_REPO"
DISTINCT_CLONES = "DISTINCT_CLONES"
NOT_A_REPO = "NOT_A_REPO"

#: Strategie, nell'ordine della catena.
NO_TRANSPORT = "NO_TRANSPORT"
FETCH_LOCAL = "FETCH_LOCAL"
CONTROLLED_COPY = "CONTROLLED_COPY"


class DistributionError(Exception):
    """Errore previsto. Porta un codice stabile, come gli errori di `rt3`."""

    def __init__(self, code, message):
        self.code = code
        super().__init__("{}: {}".format(code, message))


def git(repo, *args, **kw):
    """Esegue git in `repo`. Ritorna (returncode, stdout). Non alza."""
    check = kw.pop("check", False)
    proc = subprocess.run(
        ["git", "-C", repo] + list(args),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=120,
    )
    out = proc.stdout.decode("utf-8", "replace").strip()
    if check and proc.returncode != 0:
        raise DistributionError(
            "GIT_FAILED",
            "`git {}` in {} e' uscito {}: {}".format(
                " ".join(args), repo, proc.returncode, proc.stderr.decode("utf-8", "replace").strip()
            ),
        )
    return proc.returncode, out


def common_dir(path):
    """`--git-common-dir` ASSOLUTO e normalizzato, o None se non e' un repository.

    ⚠️ Git lo ritorna relativo (`.git`) quando lo si chiede dalla radice del repo, e
    assoluto da un worktree collegato. Confrontare le due forme come stringhe direbbe
    «cloni distinti» per due directory dello stesso repository - cioe' sbaglierebbe
    esattamente la domanda per cui questa funzione esiste.
    """
    if not os.path.isdir(path):
        return None
    code, out = git(path, "rev-parse", "--git-common-dir")
    if code != 0 or not out:
        return None
    if not os.path.isabs(out):
        out = os.path.join(path, out)
    return os.path.normcase(os.path.abspath(os.path.realpath(out)))


def classify(source, target):
    """Dice quale ramo della catena si applica. Misura, non deduce."""
    src = common_dir(source)
    if src is None:
        raise DistributionError(
            "SOURCE_NOT_A_REPO", "la sorgente {} non e' un repository Git.".format(source)
        )
    dst = common_dir(target)
    if dst is None:
        return NOT_A_REPO
    return SAME_REPO if src == dst else DISTINCT_CLONES


def reachable(repo, commit):
    """Il commit e' gia' raggiungibile da questo repository?

    E' il predicato che sostituisce «se serve»: ritorna 0 o 1, non un giudizio.
    """
    code, _ = git(repo, "cat-file", "-e", commit + "^{commit}")
    return code == 0


def _copy_allowed_entries(source, rel):
    """Filtra la deny-list. Confronta il NOME, non il percorso: `Saved` va escluso
    ovunque compaia, non solo alla radice."""
    full = os.path.join(source, rel)
    for entry in sorted(os.listdir(full)):
        if entry in COPY_DENY:
            continue
        yield entry


def _walk_files(source, rel):
    """File da copiare sotto `rel`, come percorsi relativi alla sorgente."""
    full = os.path.join(source, rel)
    if os.path.isfile(full):
        if os.path.basename(rel) in COPY_DENY:
            return
        yield rel.replace("\\", "/")
        return
    if not os.path.isdir(full):
        return
    for entry in _copy_allowed_entries(source, rel):
        for f in _walk_files(source, os.path.join(rel, entry)):
            yield f


def distribute(source, target, commit, paths, dry_run=False, allow_copy=False):
    """Porta `commit` (limitatamente a `paths`) da `source` a `target`.

    Ritorna un dict che dichiara SEMPRE: il caso misurato, la strategia scelta, il
    perche', e l'elenco esatto dei file toccati. Una distribuzione che non dice cosa ha
    trasferito non e' verificabile.
    """
    source = os.path.abspath(source)
    target = os.path.abspath(target)
    case = classify(source, target)

    result = {
        "source": source,
        "target": target,
        "commit": commit,
        "case": case,
        "paths": list(paths),
        "dryRun": bool(dry_run),
        "files": [],
    }

    if not reachable(source, commit):
        raise DistributionError(
            "COMMIT_NOT_IN_SOURCE",
            "il commit {} non esiste nella sorgente {}.".format(commit, source),
        )

    # ---- ramo A: stesso repository ------------------------------------
    if case == SAME_REPO:
        result["strategy"] = NO_TRANSPORT
        result["alreadyReachable"] = reachable(target, commit)
        result["reason"] = (
            "sorgente e bersaglio sono lo STESSO repository (git-common-dir identico): "
            "gli oggetti sono gia' condivisi e non c'e' niente da trasportare. "
            "L'integrazione e' un merge o un cherry-pick locale, e non la fa questo "
            "strumento: sposterebbe HEAD sotto chi possiede il ramo."
        )
        return result

    # ---- ramo B: cloni distinti ---------------------------------------
    if case == DISTINCT_CLONES:
        result["strategy"] = FETCH_LOCAL
        result["reason"] = (
            "cloni distinti (git-common-dir diverso). Il trasporto resta su disco: "
            "`git fetch` da un PATH locale, mai da un URL remoto."
        )
        if dry_run:
            result["files"] = sorted(_files_in_commit(source, commit, paths))
            return result

        # ⚠️ La sorgente e' un PATH, non un URL: e' cio' che tiene il trasporto locale.
        git(target, "fetch", source, commit, check=True)
        result["fetched"] = True
        result["files"] = _materialize(source, target, commit, paths)
        result["reachableAfter"] = reachable(target, commit)
        if not result["reachableAfter"]:
            raise DistributionError(
                "FETCH_INCOMPLETE",
                "dopo il fetch il commit {} non risulta raggiungibile da {}.".format(
                    commit, target
                ),
            )
        return result

    # ---- ramo C: fallback ---------------------------------------------
    # ⛔ Il controllo negativo che rende il fallback un'ULTIMA risorsa e non una
    # scorciatoia: se il bersaglio e' un repository, la copia si rifiuta. Senza questo,
    # «copia controllata come fallback» passerebbe sempre, e nessun test potrebbe
    # distinguere il fallback dall'abitudine.
    if not allow_copy:
        raise DistributionError(
            "COPY_NOT_AUTHORIZED",
            "il bersaglio {} non e' un repository Git e la copia non e' stata "
            "autorizzata. La copia e' l'ultima risorsa: passare --allow-copy solo "
            "dopo aver constatato che Git non e' utilizzabile.".format(target),
        )

    result["strategy"] = CONTROLLED_COPY
    result["reason"] = (
        "il bersaglio non e' un repository Git: ne' il ramo A ne' il ramo B sono "
        "applicabili. Copia controllata, con deny-list esplicita."
    )
    result["denied"] = list(COPY_DENY)
    files = []
    for p in paths:
        files.extend(_walk_files(source, p))
    files = sorted(files)
    result["files"] = files
    if not dry_run:
        for rel in files:
            src = os.path.join(source, rel)
            dst = os.path.join(target, rel)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(src, dst)
    return result


def _files_in_commit(source, commit, paths):
    code, out = git(source, "ls-tree", "-r", "--name-only", commit, "--", *paths)
    if code != 0:
        return []
    return [line for line in out.splitlines() if line.strip()]


def _materialize(source, target, commit, paths):
    """Scrive i file del commit nel working tree del bersaglio.

    ⚠️ `git archive | tar` e non `git checkout <sha> -- <path>`: il secondo scrive
    nell'INDICE del bersaglio, e in un checkout condiviso un `git commit` di un'altra
    sessione assorbirebbe quei file nel suo commit.
    """
    files = _files_in_commit(source, commit, paths)
    if not files:
        return []
    archive = subprocess.run(
        ["git", "-C", source, "archive", commit] + list(paths),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=300,
    )
    if archive.returncode != 0:
        raise DistributionError(
            "ARCHIVE_FAILED",
            archive.stderr.decode("utf-8", "replace").strip(),
        )
    extract = subprocess.run(
        ["tar", "-x", "-C", target],
        input=archive.stdout,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=300,
    )
    if extract.returncode != 0:
        raise DistributionError(
            "EXTRACT_FAILED", extract.stderr.decode("utf-8", "replace").strip()
        )
    return sorted(files)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="propaga un commit fra workspace, scegliendo la strada dai fatti"
    )
    parser.add_argument("--from", dest="source", default=os.getcwd())
    parser.add_argument("--to", dest="target", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--paths", nargs="+", default=["tools/rt3"])
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument(
        "--allow-copy",
        action="store_true",
        help="autorizza la copia controllata: solo dopo aver constatato che Git non e' "
        "utilizzabile sul bersaglio",
    )
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)

    try:
        result = distribute(
            args.source,
            args.target,
            args.commit,
            args.paths,
            dry_run=args.dry_run,
            allow_copy=args.allow_copy,
        )
    except DistributionError as exc:
        sys.stderr.write(str(exc) + "\n")
        return 2

    if args.json:
        import json

        sys.stdout.write(json.dumps(result, indent=2, ensure_ascii=False) + "\n")
        return 0

    print("caso      : {}".format(result["case"]))
    print("strategia : {}".format(result["strategy"]))
    print("perche'   : {}".format(result["reason"]))
    if result["strategy"] == NO_TRANSPORT:
        print(
            "commit gia' raggiungibile dal bersaglio: {}".format(
                "si" if result.get("alreadyReachable") else "no - serve un merge locale"
            )
        )
    print("file ({}){}:".format(len(result["files"]), " [dry-run]" if args.dry_run else ""))
    for f in result["files"]:
        print("  " + f)
    return 0


if __name__ == "__main__":
    sys.exit(main())
