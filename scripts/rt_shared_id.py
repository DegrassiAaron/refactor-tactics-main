#!/usr/bin/env python3
"""Allocatore atomico degli ID condivisi (`D-nnn` del Decision Log).

Nasce dalla **sedicesima** collisione di contatore del progetto. Le prime quindici sono registrate in
fondo a `docs/decisions/RT_PDR_00_Decision_Log.md`, e tutte hanno la stessa forma: due sessioni leggono
lo stesso «ultimo numero», scelgono lo stesso successivo, e chi arriva secondo rinumera. La mitigazione
scritta in `AGENTS.md` — *«rifai il controllo subito prima del merge»* — riduce il danno ma non elimina
la race: **due processi possono osservare lo stesso stato e decidere lo stesso ID**. Serve una sezione
critica vera, ed e' quello che questo script e'.

Uso:
    python scripts/rt_shared_id.py reserve D                     # stampa l'ID da usare, e nient'altro
    python scripts/rt_shared_id.py reserve D --reason "#621 bake" --count 3
    python scripts/rt_shared_id.py status                        # stato e reservation di questo clone
    python scripts/rt_shared_id.py check                         # gate: duplicati e formati nel Decision Log
    python scripts/rt_shared_id.py audit-refs                    # gate: stesso ID, decisioni diverse, fra i ref

`check` e `audit-refs` escono con codice 1 quando trovano un difetto. `reserve` stampa **solo** l'ID su
stdout, cosi' e' consumabile da uno script; le diagnostiche vanno su stderr.

## Dove vive lo stato, e perche' non e' versionato

Sotto `$(git rev-parse --git-common-dir)/rt-shared-ids/`. **Common dir**, non worktree: e' l'unica
directory che tutti i worktree dello stesso clone condividono, quindi e' l'unico posto dove un lock e'
visibile a tutte le sessioni. Un `next-id.txt` versionato sarebbe l'opposto — una copia per worktree, e
un punto di merge conflict in piu'.

## Cosa NON risolve

L'atomicita' si ferma al clone. Due **PC** diversi, o due cloni sullo stesso PC, non condividono il
common dir e possono ancora emettere lo stesso ID: per quel caso c'e' `audit-refs`, che e' una **difesa
a valle** (trova la collisione prima del merge) e non una prevenzione. Il limite e' dichiarato, non
aggirato.
"""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
import time

# Percorso del Decision Log, relativo alla radice del repository.
DECISION_LOG = "docs/decisions/RT_PDR_00_Decision_Log.md"

# Il ref rispetto a cui si giudica «gia' integrato». E' anche l'unico che il filtro non puo' saltare.
BASE_REF = "origin/main"

STATE_DIRNAME = "rt-shared-ids"
STATE_FILE = "state.json"
LOCK_FILE = "allocator.lock"
SCHEMA_VERSION = 1

# Quanto aspettare il lock prima di arrendersi. L'allocazione dura millisecondi: se dopo trenta secondi
# non e' ancora libero, qualcosa e' bloccato e va detto invece di attendere in eterno.
LOCK_TIMEOUT_S = 30.0
LOCK_RETRY_S = 0.02

# --- Parser delle dichiarazioni canoniche -----------------------------------------------------------
#
# Una **dichiarazione** e' una riga della tabella del Decision Log la cui prima cella contiene solo
# l'ID, eventualmente decorato. Un **riferimento** — `[D-120](RT_PDR_00_Decision_Log.md)` dentro il
# testo di un'altra decisione — non lo e'. Nel file reale il rapporto e' 131 dichiarazioni su 468
# occorrenze: un parser che contasse le occorrenze sbaglierebbe di un fattore tre.
#
# ⚠️ I marcatori si annidano nei due versi, e questo ha gia' morso: la spec elencava `~~D-002~~` e
# `**D-131**`, ma la riga di `D-044` e' `| ~~**D-044**~~ |` — strike ESTERNO al bold. Un pattern scritto
# sugli esempi perdeva quella riga sola, e proprio quella di una decisione ritirata. Per questo i
# marcatori si accettano in qualunque ordine e quantita', invece di elencarne le combinazioni.
_MARKERS = r"(?:\*\*|~~)*"
DECLARATION = re.compile(
    r"^\|\s*" + _MARKERS + r"\s*(?P<id>D-\d{3,})\s*" + _MARKERS + r"\s*\|",
)

# Rete di sicurezza del pattern qui sopra: cattura le forme che *sembrano* un ID nella prima cella ma
# non rispettano il formato canonico (`D-44` senza zero-padding, `D_045`, trattino unicode). Se una riga
# matcha questa e non `DECLARATION`, e' un errore di formato — non una riga da ignorare in silenzio.
MALFORMED = re.compile(
    r"^\|\s*" + _MARKERS + r"\s*(?P<id>D[-‐-―_ ]?\s*\d+[A-Za-z0-9]*)\s*" + _MARKERS + r"\s*\|",
)

# Il fingerprint serve a distinguere «la stessa decisione ereditata da main, poi rivista» da «due
# decisioni diverse sotto lo stesso numero». Si calcola sulla **tesi**: il primo segmento in grassetto
# della cella, che nel formato del Decision Log e' l'enunciato della decisione.
#
# ⚠️ La prima versione usava il testo intero troncato a 160 caratteri, e sui ref reali ha prodotto un
# falso positivo su `D-031`: `wip/icon-visual-language` porta la **stessa** decisione con il corpo
# ampliato (557 contro 591 caratteri, tesi identica). Un gate che segnala ogni revisione di una
# decisione ereditata suona a ogni branch, e un gate che suona sempre viene spento. La tesi invece
# separa i casi reali: `D-091` e `D-132` restano rossi perche' li' gli enunciati sono proprio diversi.
FINGERPRINT_PREFIX = 160
THESIS = re.compile(r"\*\*(.+?)\*\*", re.DOTALL)


class Declaration:
    """Una riga canonica: quale ID dichiara, dove, e con quale contenuto."""

    def __init__(self, decision_id, line_no, text):
        self.id = decision_id
        self.num = int(decision_id.split("-")[1])
        self.line_no = line_no
        self.text = text

    @property
    def fingerprint(self):
        cells = self.text.split("|")
        body = cells[2] if len(cells) > 2 else ""
        thesis = THESIS.search(body)
        # Senza grassetto — forma rara, ma esiste — si ricade sul prefisso del corpo: meglio un
        # confronto piu' grossolano che nessun confronto.
        subject = thesis.group(1) if thesis else body[:FINGERPRINT_PREFIX]
        normalized = re.sub(r"[^0-9a-z]+", " ", subject.lower()).strip()
        return hashlib.sha1(normalized.encode("utf-8")).hexdigest()[:12]

    def summary(self, width=60):
        cells = self.text.split("|")
        body = re.sub(r"\s+", " ", cells[2] if len(cells) > 2 else "").strip()
        return body[:width] + ("..." if len(body) > width else "")


def parse_declarations(content):
    """Estrae le dichiarazioni canoniche dal testo del Decision Log."""
    out = []
    for line_no, line in enumerate(content.splitlines(), 1):
        match = DECLARATION.match(line)
        if match:
            out.append(Declaration(match.group("id"), line_no, line))
    return out


def parse_malformed(content):
    """Righe la cui prima cella sembra un ID ma non rispetta `D-` + almeno tre cifre."""
    out = []
    for line_no, line in enumerate(content.splitlines(), 1):
        if DECLARATION.match(line):
            continue
        match = MALFORMED.match(line)
        if match:
            out.append((match.group("id"), line_no))
    return out


# --- Git ---------------------------------------------------------------------------------------------

def git(args, cwd=None, check=True):
    """Esegue git e restituisce stdout. Nessuna operazione di rete: questo script non fa fetch."""
    proc = subprocess.run(
        ["git"] + args, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    if check and proc.returncode != 0:
        raise RuntimeError(
            "git %s -> exit %d: %s" % (" ".join(args), proc.returncode, proc.stderr.decode("utf-8", "replace").strip())
        )
    return proc.stdout.decode("utf-8", "replace")


def repo_root(cwd=None):
    return git(["rev-parse", "--show-toplevel"], cwd=cwd).strip()


def common_dir(cwd=None):
    """Il git dir condiviso da tutti i worktree del clone.

    `--git-common-dir` puo' tornare un percorso relativo alla cwd: va risolto **subito**, altrimenti due
    worktree con cwd diverse calcolerebbero due directory di stato diverse, che e' esattamente il difetto
    che questo script esiste per non avere.
    """
    raw = git(["rev-parse", "--git-common-dir"], cwd=cwd).strip()
    base = cwd or os.getcwd()
    return os.path.abspath(os.path.join(base, raw)) if not os.path.isabs(raw) else raw


def current_branch(cwd=None):
    name = git(["rev-parse", "--abbrev-ref", "HEAD"], cwd=cwd).strip()
    return None if name == "HEAD" else name


def head_sha(cwd=None):
    try:
        return git(["rev-parse", "--short", "HEAD"], cwd=cwd).strip()
    except RuntimeError:
        return None  # repository senza commit


def read_blob(ref, path, cwd=None):
    """Contenuto di un file a un dato ref, o None se il ref o il file non esistono."""
    proc = subprocess.run(
        ["git", "show", "%s:%s" % (ref, path)], cwd=cwd,
        stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
    )
    if proc.returncode != 0:
        return None
    return proc.stdout.decode("utf-8", "replace")


# --- Lock cross-platform ------------------------------------------------------------------------------

class FileLock:
    """Lock esclusivo fra processi, nel git common dir.

    Windows usa `msvcrt.locking`, POSIX `fcntl.flock`. Entrambi in modalita' non bloccante dentro un
    ciclo di retry: la variante bloccante di `msvcrt` si arrende da sola dopo dieci secondi con un errore
    che non distingue «occupato» da «rotto», e un timeout esplicito e' diagnosticabile.

    ⚠️ Questi lock sono **per descrittore di file**, quindi serializzano processi, non thread dello
    stesso processo. E' il motivo per cui il test di concorrenza usa `subprocess` e non `threading`: un
    test a thread proverebbe qualcosa che il lock non promette.
    """

    def __init__(self, path, timeout=LOCK_TIMEOUT_S):
        self.path = path
        self.timeout = timeout
        self.fd = None

    def __enter__(self):
        self.fd = os.open(self.path, os.O_RDWR | os.O_CREAT, 0o600)
        # msvcrt blocca un intervallo di byte: il file non puo' restare vuoto.
        if os.fstat(self.fd).st_size == 0:
            os.write(self.fd, b"0")
            os.lseek(self.fd, 0, os.SEEK_SET)
        deadline = time.monotonic() + self.timeout
        while True:
            try:
                self._acquire()
                return self
            except OSError:
                if time.monotonic() >= deadline:
                    os.close(self.fd)
                    self.fd = None
                    raise RuntimeError(
                        "lock non acquisito entro %.0fs: %s" % (self.timeout, self.path)
                    )
                time.sleep(LOCK_RETRY_S)

    def _acquire(self):
        if os.name == "nt":
            import msvcrt
            os.lseek(self.fd, 0, os.SEEK_SET)
            msvcrt.locking(self.fd, msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(self.fd, fcntl.LOCK_EX | fcntl.LOCK_NB)

    def __exit__(self, *exc):
        try:
            if os.name == "nt":
                import msvcrt
                os.lseek(self.fd, 0, os.SEEK_SET)
                msvcrt.locking(self.fd, msvcrt.LK_UNLCK, 1)
            else:
                import fcntl
                fcntl.flock(self.fd, fcntl.LOCK_UN)
        finally:
            os.close(self.fd)
            self.fd = None
        return False


# --- Stato --------------------------------------------------------------------------------------------

def state_dir(cwd=None):
    path = os.path.join(common_dir(cwd), STATE_DIRNAME)
    os.makedirs(path, exist_ok=True)
    return path


def load_state(directory):
    path = os.path.join(directory, STATE_FILE)
    if not os.path.exists(path):
        return {"schema_version": SCHEMA_VERSION, "namespaces": {}, "reservations": []}
    with open(path, "r", encoding="utf-8") as handle:
        try:
            return json.load(handle)
        except ValueError as exc:
            raise RuntimeError(
                "state.json illeggibile (%s). Non va riparato a intuito: rinominalo come backup e "
                "rilancia — il bootstrap ricostruisce da capo dal massimo canonico, e il contatore "
                "restera' monotono perche' non riempie mai i buchi. Vedi "
                "docs/technical/tooling/workflow-parallel-claude.md §Recovery." % exc
            )


def save_state(directory, state):
    """Scrittura atomica: temporaneo nella stessa directory, fsync, `os.replace`.

    Un crash a meta' non deve lasciare mezzo JSON — che sarebbe indistinguibile da una corruzione e
    costringerebbe a un recovery manuale per un'operazione che dura millisecondi.
    """
    fd, tmp = tempfile.mkstemp(dir=directory, prefix=".state-", suffix=".json")
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as handle:
            json.dump(state, handle, indent=2, ensure_ascii=False)
            handle.write("\n")
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(tmp, os.path.join(directory, STATE_FILE))
    except BaseException:
        if os.path.exists(tmp):
            os.unlink(tmp)
        raise


def worktree_paths(cwd=None):
    """Tutti i working tree del clone, incluso quello corrente.

    E' l'unico modo di vedere il lavoro che una sessione parallela ha scritto ma non ancora committato,
    ed e' proprio la finestra in cui le collisioni nascono: fra «ho scelto il numero» e «l'ho pushato».
    """
    paths = []
    for line in git(["worktree", "list", "--porcelain"], cwd=cwd).splitlines():
        if line.startswith("worktree "):
            paths.append(line[len("worktree "):].strip())
    return paths or [repo_root(cwd)]


def all_refs(cwd=None):
    """Ref locali e di origin, senza l'alias `origin` di `refs/remotes/origin/HEAD`."""
    out = []
    for line in git(["for-each-ref", "--format=%(refname:short)",
                     "refs/heads", "refs/remotes/origin"], cwd=cwd).splitlines():
        name = line.strip()
        if name and name != "origin" and not name.endswith("/HEAD"):
            out.append(name)
    return out


def is_merged(ref, cwd=None):
    """Il ref e' gia' antenato di `origin/main`, cioe' la sua storia e' interamente integrata."""
    proc = subprocess.run(
        ["git", "merge-base", "--is-ancestor", ref, BASE_REF], cwd=cwd,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
    )
    return proc.returncode == 0


def canonical_max(cwd=None):
    """Il massimo ID dichiarato su **tutti** i ref, piu' il working tree corrente.

    Guardare solo il working tree sarebbe sbagliato in modo silenzioso: due worktree su branch diversi
    vedrebbero due massimi diversi, e il primo che bootstrappa fisserebbe il contatore piu' basso.

    ⚠️ Nemmeno `origin/main` basta, e il repository lo ha dimostrato il giorno dell'introduzione: al
    momento del primo `reserve`, `origin/main` era fermo a `D-131` mentre **due branch aperti** avevano
    gia' rivendicato `D-132` — l'allocatore avrebbe emesso proprio quel numero, diventando la terza
    sessione a collidere sullo stesso ID. Un branch aperto e' una rivendicazione a tutti gli effetti,
    anche se non e' ancora mergiato: il massimo si prende dall'unione dei ref.

    Per la stessa ragione si leggono i Decision Log **su disco di tutti i worktree** del clone, non solo
    quello corrente: una sessione parallela che ha gia' scritto il proprio numero ma non l'ha ancora
    committato lo ha rivendicato di fatto. Anche questo e' misurato, e nello stesso momento: il primo
    `reserve` ha emesso `D-134` mentre un worktree accanto teneva `D-134` fra le modifiche non
    committate. Un ref non lo avrebbe mai mostrato.

    Restano fuori — e sono i limiti dichiarati — un ID su un clone **mai fetchato**, e un file aperto in
    un editor e non ancora salvato. Per il primo c'e' `audit-refs`, che e' una difesa a valle; per il
    secondo non c'e' difesa che non sia salvare.
    """
    best = 0
    for tree in worktree_paths(cwd):
        local_path = os.path.join(tree, DECISION_LOG)
        if os.path.exists(local_path):
            with open(local_path, "r", encoding="utf-8") as handle:
                best = max([d.num for d in parse_declarations(handle.read())] + [best])
    for ref in all_refs(cwd):
        content = read_blob(ref, DECISION_LOG, cwd=cwd)
        if content:
            best = max([d.num for d in parse_declarations(content)] + [best])
    return best


# --- Comandi ------------------------------------------------------------------------------------------

def cmd_reserve(args):
    directory = state_dir()
    lock_path = os.path.join(directory, LOCK_FILE)
    if args.count < 1:
        sys.stderr.write("--count deve essere >= 1\n")
        return 2

    branch = current_branch()
    sha = head_sha()
    stamp = time.strftime("%Y-%m-%dT%H:%M:%S%z")

    # Tutta la sequenza sta dentro il lock: leggere fuori e scrivere dentro ricrearebbe la race.
    with FileLock(lock_path):
        state = load_state(directory)
        namespace = state.setdefault("namespaces", {}).setdefault(args.namespace, {"last_issued": 0})

        # Il massimo canonico si rilegge a **ogni** allocazione, non solo al bootstrap: dopo un pull che
        # porta decisioni da un altro clone, un contatore fermo al proprio ultimo valore riemetterebbe
        # ID gia' usati altrove. Il contatore non scende mai — i gap costano zero, il riuso costa
        # ambiguita'.
        floor = max(namespace.get("last_issued", 0), canonical_max())
        issued = []
        for offset in range(1, args.count + 1):
            number = floor + offset
            issued.append("%s-%03d" % (args.namespace, number))
        namespace["last_issued"] = floor + args.count

        for decision_id in issued:
            entry = {"id": decision_id, "created_at": stamp, "branch": branch, "base_sha": sha}
            if args.reason:
                entry["reason"] = args.reason
            state.setdefault("reservations", []).append(entry)
        save_state(directory, state)

    for decision_id in issued:
        print(decision_id)
    return 0


def cmd_release(args):
    """Cede una reservation **senza** liberare l'ID.

    Nasce da un falso positivo osservato il giorno stesso dell'introduzione. Avevo riservato `D-134` e
    poi ceduto a un'altra sessione che l'aveva gia' scritto; la reservation pero' restava registrata a
    nome del mio branch, e la regola `reserved-by-other-branch` di `check` avrebbe segnalato come
    collisione l'uso **legittimo** che quella sessione ne stava facendo.

    ⚠️ `last_issued` non si tocca: cedere non e' liberare. L'ID resta bruciato per chiunque altro, e chi
    lo sta gia' usando lo tiene. Serve solo a togliere di mezzo una diagnostica diventata falsa.
    """
    directory = state_dir()
    with FileLock(os.path.join(directory, LOCK_FILE)):
        state = load_state(directory)
        before = state.get("reservations", [])
        kept = [entry for entry in before if entry.get("id") != args.id]
        if len(kept) == len(before):
            sys.stderr.write("nessuna reservation per %s su questo clone.\n" % args.id)
            return 1
        state["reservations"] = kept
        save_state(directory, state)
    print("%s ceduto: %d reservation rimosse, il contatore non e' sceso." % (args.id, len(before) - len(kept)))
    return 0


def cmd_status(args):
    directory = state_dir()
    state = load_state(directory)
    print("stato:        %s" % os.path.join(directory, STATE_FILE))
    print("branch:       %s" % (current_branch() or "(detached)"))
    print("max canonico: D-%03d" % canonical_max())
    for namespace, data in sorted(state.get("namespaces", {}).items()):
        print("namespace %s: last_issued=%d" % (namespace, data.get("last_issued", 0)))

    reservations = state.get("reservations", [])
    if not reservations:
        print("nessuna reservation registrata su questo clone.")
        return 0

    root = repo_root()
    log_path = os.path.join(root, DECISION_LOG)
    declared = set()
    if os.path.exists(log_path):
        with open(log_path, "r", encoding="utf-8") as handle:
            declared = {d.id for d in parse_declarations(handle.read())}

    print("\nreservation (%d):" % len(reservations))
    for entry in reservations:
        # «orfana» qui significa solo «non ancora dichiarata nel Decision Log di QUESTO worktree»: una
        # reservation usata su un altro branch risulta orfana finche' non e' mergiata. E' diagnostica,
        # non un verdetto — e in nessun caso l'ID torna disponibile.
        mark = "usata " if entry["id"] in declared else "orfana"
        print("  %s  %s  branch=%s  %s" % (
            mark, entry["id"], entry.get("branch") or "-", entry.get("reason", "")))
    return 0


def cmd_check(args):
    root = repo_root()
    log_path = os.path.join(root, DECISION_LOG)
    with open(log_path, "r", encoding="utf-8") as handle:
        content = handle.read()

    declarations = parse_declarations(content)
    errors = 0

    by_id = {}
    for declaration in declarations:
        by_id.setdefault(declaration.id, []).append(declaration)
    for decision_id, group in sorted(by_id.items()):
        if len(group) > 1:
            errors += 1
            sys.stderr.write("ERROR decision-id duplicate: %s\n" % decision_id)
            for declaration in group:
                sys.stderr.write('  line %d: "%s"\n' % (declaration.line_no, declaration.summary()))
            sys.stderr.write(
                "Use a new reservation; do not renumber by global search/replace.\n"
            )

    for bad_id, line_no in parse_malformed(content):
        errors += 1
        sys.stderr.write(
            "ERROR decision-id malformed: %r at line %d (atteso `D-` + almeno tre cifre)\n"
            % (bad_id, line_no)
        )

    # Terza regola: una dichiarazione **nuova su questo branch** che usa un ID riservato da un'altra
    # sessione. Il confronto con `origin/main` e' cio' che la rende silenziosa dopo il merge — senza,
    # ogni decisione mergiata resterebbe segnalata per sempre e il gate verrebbe spento.
    directory = state_dir()
    state = load_state(directory)
    branch = current_branch()
    base = read_blob(BASE_REF, DECISION_LOG)
    if base is not None:
        on_main = {d.id for d in parse_declarations(base)}
        reserved_elsewhere = {
            entry["id"]: entry for entry in state.get("reservations", [])
            if entry.get("branch") and entry.get("branch") != branch
        }
        for declaration in declarations:
            if declaration.id in on_main:
                continue
            owner = reserved_elsewhere.get(declaration.id)
            if owner:
                errors += 1
                sys.stderr.write(
                    "ERROR decision-id reserved-by-other-branch: %s at line %d\n"
                    "  riservato da `%s`%s\n"
                    "  Riserva un ID nuovo: python scripts/rt_shared_id.py reserve D\n"
                    % (declaration.id, declaration.line_no, owner.get("branch"),
                       " (%s)" % owner["reason"] if owner.get("reason") else "")
                )

    if errors:
        sys.stderr.write("\n%d problema/i.\n" % errors)
        return 1
    print("check: %d dichiarazioni canoniche, nessun duplicato." % len(declarations))
    return 0


def cmd_audit_refs(args):
    """Confronta le dichiarazioni fra i ref locali e quelli di origin.

    Lo stesso ID su piu' ref e' **normale** quando la decisione e' ereditata: il fingerprint sara' lo
    stesso. Due testi diversi sotto lo stesso numero sono una collisione.

    ⚠️ Legge **ref**, quindi non vede il lavoro non ancora committato di un altro worktree. Una
    collisione che vive solo nella working directory di un'altra sessione resta invisibile fino al
    primo commit: e' il caso che il lock di `reserve` esiste per prevenire a monte.
    """
    # ⚠️ I ref gia' **antenati di `origin/main`** si saltano, e non e' un'ottimizzazione: e' cio' che
    # separa una collisione da una cicatrice. Un branch mergiato porta la storia com'era prima del
    # merge, comprese le collisioni che il merge ha gia' risolto rinumerando. Misurato su questo
    # repository: `docs/wv4-environmental` e `docs/505-pass-per-fase` (PR #524 e #525, mergiate
    # l'11 agosto) dichiarano ancora `D-091` due volte, mentre in `main` quella decisione e' `D-100`
    # da allora — rinumerata proprio per quella collisione, e registrata nelle Note. Segnalarli
    # significa chiedere di correggere qualcosa di gia' corretto, su rami che nessuno tocchera' piu':
    # dodici ref su ventidue erano in questo stato, cioe' piu' della meta' del rumore.
    #
    # Non introduce falsi negativi: la storia di un ref mergiato e' contenuta in `main`, quindi una
    # collisione ancora viva la' e' visibile a `check`, che guarda l'albero corrente.
    # ⚠️ `origin/main` e' antenato di se' stesso, quindi il filtro lo escluderebbe — e con lui il
    # termine di paragone di ogni confronto. Il test `..._con_la_stessa_collisione_suona` e' nato
    # rosso proprio su questo: senza l'eccezione il gate diventava verde su `D-132`, che e' viva.
    # Un filtro contro il rumore che si mangia il riferimento non riduce i falsi positivi: produce
    # falsi negativi, che costano molto di piu'.
    live, merged = [], []
    for ref in all_refs():
        (merged if ref != BASE_REF and is_merged(ref) else live).append(ref)
    refs = live
    seen = {}
    duplicates = []
    for ref in refs:
        content = read_blob(ref, DECISION_LOG)
        if content is None:
            continue
        per_ref = {}
        for declaration in parse_declarations(content):
            per_ref.setdefault(declaration.id, []).append(declaration)
            seen.setdefault(declaration.id, {}).setdefault(
                declaration.fingerprint, []).append((ref, declaration))
        for decision_id, group in sorted(per_ref.items()):
            if len(group) > 1:
                duplicates.append((ref, decision_id, group))

    collisions = 0
    # Lo stesso ID dichiarato due volte **nello stesso** Decision Log e' piu' grave di due branch che
    # divergono: quel file e' gia' incoerente con se' stesso, e il merge non lo puo' risolvere.
    for ref, decision_id, group in duplicates:
        collisions += 1
        sys.stderr.write("DUPLICATE %s in %s\n" % (decision_id, ref))
        for declaration in group:
            sys.stderr.write("  line %-5d %s\n" % (declaration.line_no, declaration.summary(48)))

    for decision_id, fingerprints in sorted(seen.items()):
        if len(fingerprints) < 2:
            continue
        collisions += 1
        sys.stderr.write("COLLISION %s\n" % decision_id)
        # Un ID ereditato vive su decine di ref: elencarli tutti seppellisce l'unica riga che conta,
        # cioe' quale gruppo porta la decisione diversa.
        for fingerprint, entries in sorted(fingerprints.items(), key=lambda kv: -len(kv[1])):
            names = [ref for ref, _ in entries]
            shown = ", ".join(names[:3]) + (" (+%d altri)" % (len(names) - 3) if len(names) > 3 else "")
            sys.stderr.write("  %s  %s\n" % (fingerprint, entries[0][1].summary(56)))
            sys.stderr.write("    %s\n" % shown)

    if collisions:
        sys.stderr.write(
            "\n%d collisione/i su %d ref vivi: stesso ID, decisioni diverse. Rinumera la seconda "
            "**prima** del merge e registrala nelle Note del Decision Log.\n" % (collisions, len(refs))
        )
        if merged:
            sys.stderr.write(
                "(%d ref gia' antenati di origin/main non sono stati confrontati: la loro storia e' "
                "integrata, e le collisioni che contengono sono gia' state risolte al merge.)\n"
                % len(merged)
            )
        return 1
    print("audit-refs: %d ref confrontati, %d ID, nessuna collisione.%s" % (
        len(refs), len(seen),
        " (%d ref gia' in main, saltati)" % len(merged) if merged else ""))
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog="rt_shared_id.py",
        description="Allocatore atomico e validator degli ID condivisi del Decision Log.",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    reserve = sub.add_parser("reserve", help="riserva il prossimo ID libero e stampalo")
    reserve.add_argument("namespace", nargs="?", default="D", help="spazio dei nomi (per ora solo D)")
    reserve.add_argument("--reason", help="a cosa serve: issue, checkpoint, task")
    reserve.add_argument("--count", type=int, default=1, help="quanti ID contigui riservare")
    reserve.set_defaults(func=cmd_reserve)

    release = sub.add_parser("release", help="cede una reservation, senza liberare l'ID")
    release.add_argument("id", help="l'ID ceduto, es. D-134")
    release.set_defaults(func=cmd_release)

    status = sub.add_parser("status", help="stato del contatore e reservation di questo clone")
    status.set_defaults(func=cmd_status)

    check = sub.add_parser("check", help="gate: duplicati e formati nel Decision Log corrente")
    check.set_defaults(func=cmd_check)

    audit = sub.add_parser("audit-refs", help="gate: stesso ID con decisioni diverse fra i ref")
    audit.set_defaults(func=cmd_audit_refs)

    # Le decisioni contengono accenti e simboli; su una console Windows in cp1252 una stampa non
    # codificabile abortirebbe il gate con un UnicodeEncodeError, che si legge come un difetto dello
    # script invece che come un carattere fuori tabella.
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(errors="replace")

    args = parser.parse_args(argv)
    try:
        return args.func(args)
    except RuntimeError as exc:
        sys.stderr.write("ERROR %s\n" % exc)
        return 2


if __name__ == "__main__":
    sys.exit(main())
