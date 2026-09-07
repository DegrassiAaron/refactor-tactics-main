"""Roadmap RT3: il PLAN. Parsing, normalizzazione e validazione.

Quattro piani che RT3 tiene separati, e che questo modulo non confonde mai:

    PLAN        cosa c'e' da fare e cosa dipende da cosa   <- questo file, versionato
    SCHEDULE    cosa e' eseguibile ADESSO                  <- planner.py, derivato
    ASSIGNMENT  chi lo prende                              <- planner.py, derivato
    RUNTIME     cosa e' stato fatto davvero                <- store.py, NON versionato

🔴 Il PLAN non contiene stato. In una roadmap non si scrive `state: VALIDATED`, e il
parser lo RIFIUTA: se lo stato vivesse nel file versionato, avanzare una issue sarebbe
un commit, due sessioni che avanzano due issue sarebbero un conflitto di merge, e il
control plane sarebbe una seconda autorita' accanto a Git. Lo stato vive nel database
per macchina, come le sessioni e la mailbox.

Il documento e' letto da `yamlmini`, non da PyYAML: vedi il modulo per il perche'.

## Forma

    roadmapSchemaVersion: 1
    id: rt3-smoke-multi-epic
    name: ...
    resources:
      workspaces:
        DEV: { writerCapacity: 1 }
      temporaryWorktrees: { capacity: 1 }
      unrealEditor: { leaseCapacity: 1 }
    wip:
      perWorkspace: 2
      global: 4
    epics:
      - id: EPIC-B
        homeWork: DEV
        issues:
          - id: B2
            estimate: 3
            requires: [ { item: B1, gate: VALIDATED } ]

## Chiavi degli item

La chiave canonica di una issue e' `EPIC/ISSUE` - `EPIC-B/B2`. Un `requires` puo'
nominare la forma breve (`B2`) quando non e' ambigua sull'intera roadmap; se lo e', la
validazione lo dice e nomina le candidate, invece di scegliere.
"""

import hashlib
import json

from . import ROADMAP_SCHEMA_VERSION
from .errors import Rt3Error
from .model import WORKSPACE_GROUPS
from .yamlmini import YamlError, parse_file

__all__ = [
    "ROADMAP_SCHEMA_VERSION",
    "GATES",
    "RESOURCE_KINDS",
    "Roadmap",
    "RoadmapError",
    "Problem",
    "load",
    "load_document",
    "normalize",
    "content_hash",
    "dumps",
]

#: I gate che una dipendenza puo' esigere. Sono stati di PROGRESSO, non di readiness:
#: esigere `READY` non avrebbe senso, perche' READY e' derivato e non e' un traguardo.
GATES = ("VALIDATED", "DONE")

#: Risorse esclusive dichiarabili da una issue. Chiusa apposta: una risorsa scritta a
#: mano e non riconosciuta verrebbe ignorata dallo scheduler, e la issue verrebbe
#: pianificata come se non ne avesse bisogno - cioe' il difetto che la dichiarazione
#: esisteva per evitare.
RESOURCE_KINDS = ("UNREAL_EDITOR",)

#: Chiavi che una roadmap NON puo' portare: sono runtime, e il loro posto e' il
#: database. Enumerate per dare un errore che spiega, invece di un "chiave ignota".
_RUNTIME_KEYS = {
    "state": "lo stato di avanzamento e' runtime: vive nel control plane, non nel file",
    "status": "lo stato di avanzamento e' runtime: vive nel control plane, non nel file",
    "progress": "l'avanzamento e' runtime: vive nel control plane, non nel file",
    "assignee": "l'assegnazione e' runtime: la produce il planner, non il file",
    "assignedTo": "l'assegnazione e' runtime: la produce il planner, non il file",
    "session": "le sessioni sono runtime: vivono nel control plane",
    "candidate": "i candidate sono runtime: vivono nel control plane",
}

_EPIC_KEYS = {"id", "title", "homeWork", "issues", "note"}
_ISSUE_KEYS = {
    "id",
    "title",
    "executionWork",
    "estimate",
    "capabilities",
    "writeSet",
    "resources",
    "requires",
    "requiredGate",
    "note",
    "parallelizable",
}
_ROOT_KEYS = {"roadmapSchemaVersion", "id", "name", "note", "resources", "wip", "epics"}


class RoadmapError(Rt3Error):
    """Roadmap non caricabile. Porta l'elenco COMPLETO dei problemi, non il primo.

    Un errore per volta costringe a un ciclo correggi-riesegui lungo quanto il numero
    di errori; e su una roadmap scritta a mano gli errori arrivano a grappoli.
    """

    code = "RT3_ROADMAP_INVALID"
    exit_code = 20

    def __init__(self, message, problems=None):
        super().__init__(message)
        self.problems = list(problems or [])


class Problem(object):
    """Un rilievo della validazione. `level` e' ERROR o WARNING."""

    __slots__ = ("level", "code", "where", "message")

    def __init__(self, level, code, where, message):
        self.level = level
        self.code = code
        self.where = where
        self.message = message

    def as_dict(self):
        return {
            "level": self.level,
            "code": self.code,
            "where": self.where,
            "message": self.message,
        }

    def __str__(self):
        return "{} {} [{}] {}".format(self.level, self.code, self.where, self.message)


class Issue(object):
    """Una unita' di lavoro. Immutabile per intenzione: il PLAN non cambia in memoria."""

    __slots__ = (
        "key",
        "id",
        "epic_id",
        "title",
        "home_work",
        "execution_work",
        "estimate",
        "capabilities",
        "write_set",
        "resources",
        "requires",
        "note",
    )

    def __init__(self, **kw):
        for slot in self.__slots__:
            setattr(self, slot, kw.get(slot))

    @property
    def needs_unreal(self):
        return "UNREAL_EDITOR" in (self.resources or ())

    def as_dict(self):
        return {
            "key": self.key,
            "id": self.id,
            "epicId": self.epic_id,
            "title": self.title,
            "homeWork": self.home_work,
            "executionWork": self.execution_work,
            "estimate": self.estimate,
            "capabilities": list(self.capabilities or []),
            "writeSet": list(self.write_set or []),
            "resources": list(self.resources or []),
            "requires": [dict(r) for r in (self.requires or [])],
            "note": self.note,
        }


class Epic(object):
    __slots__ = ("id", "title", "home_work", "issue_keys", "note")

    def __init__(self, **kw):
        for slot in self.__slots__:
            setattr(self, slot, kw.get(slot))

    def as_dict(self):
        return {
            "id": self.id,
            "title": self.title,
            "homeWork": self.home_work,
            "issues": list(self.issue_keys or []),
            "note": self.note,
        }


class Roadmap(object):
    """Il PLAN normalizzato. `items` e' ordinato come nel documento: l'ordine di
    dichiarazione e' l'unico tie-break stabile che il planner puo' usare senza
    inventare una priorita' che nessuno ha scritto."""

    def __init__(
        self,
        roadmap_id,
        name,
        schema_version,
        epics,
        items,
        resources,
        wip,
        source_path=None,
        content_hash=None,
        note=None,
    ):
        self.id = roadmap_id
        self.name = name
        self.schema_version = schema_version
        self.epics = epics
        self.items = items
        self.resources = resources
        self.wip = wip
        self.source_path = source_path
        self.content_hash = content_hash
        self.note = note

    # -- accesso ----------------------------------------------------------

    def keys(self):
        return list(self.items.keys())

    def get(self, key):
        return self.items.get(key)

    def writer_capacity(self, workspace_group):
        return int(
            self.resources["workspaces"].get(workspace_group, {}).get("writerCapacity", 0)
        )

    @property
    def temporary_worktree_capacity(self):
        return int(self.resources["temporaryWorktrees"].get("capacity", 0))

    @property
    def unreal_lease_capacity(self):
        return int(self.resources["unrealEditor"].get("leaseCapacity", 0))

    # -- serializzazione --------------------------------------------------

    def as_dict(self):
        return {
            "roadmapSchemaVersion": self.schema_version,
            "id": self.id,
            "name": self.name,
            "note": self.note,
            "sourcePath": self.source_path,
            "contentHash": self.content_hash,
            "resources": self.resources,
            "wip": self.wip,
            "epics": [e.as_dict() for e in self.epics],
            "items": [i.as_dict() for i in self.items.values()],
        }

    @classmethod
    def from_dict(cls, data):
        """Ricostruisce una roadmap dal JSON salvato nel database.

        Serve perche' il control plane conserva il documento NORMALIZZATO: rileggere il
        file dal disco a ogni query lo renderebbe dipendente dal checkout da cui parte
        il comando, e i tre workspace possono avere tre versioni diverse dello stesso
        file. Cio' che e' stato caricato e' cio' che vale, finche' non si ricarica.
        """
        items = {}
        for raw in data.get("items", []):
            items[raw["key"]] = Issue(
                key=raw["key"],
                id=raw["id"],
                epic_id=raw["epicId"],
                title=raw.get("title"),
                home_work=raw.get("homeWork"),
                execution_work=raw.get("executionWork"),
                estimate=raw.get("estimate"),
                capabilities=raw.get("capabilities") or [],
                write_set=raw.get("writeSet") or [],
                resources=raw.get("resources") or [],
                requires=raw.get("requires") or [],
                note=raw.get("note"),
            )
        epics = [
            Epic(
                id=e["id"],
                title=e.get("title"),
                home_work=e.get("homeWork"),
                issue_keys=e.get("issues") or [],
                note=e.get("note"),
            )
            for e in data.get("epics", [])
        ]
        return cls(
            roadmap_id=data["id"],
            name=data.get("name"),
            schema_version=data.get("roadmapSchemaVersion"),
            epics=epics,
            items=items,
            resources=data.get("resources") or {},
            wip=data.get("wip") or {},
            source_path=data.get("sourcePath"),
            content_hash=data.get("contentHash"),
            note=data.get("note"),
        )


# ---------------------------------------------------------------------------
# Normalizzazione
# ---------------------------------------------------------------------------


def _err(problems, code, where, message):
    problems.append(Problem("ERROR", code, where, message))


def _warn(problems, code, where, message):
    problems.append(Problem("WARNING", code, where, message))


def _check_runtime_keys(mapping, where, problems):
    for key in mapping or ():
        if key in _RUNTIME_KEYS:
            _err(
                problems,
                "ROADMAP_RUNTIME_KEY_IN_PLAN",
                where,
                "chiave `{}` non ammessa: {}.".format(key, _RUNTIME_KEYS[key]),
            )


def _check_unknown_keys(mapping, allowed, where, problems):
    for key in mapping or ():
        if key in _RUNTIME_KEYS:
            continue  # gia' segnalata, e con un messaggio migliore
        if key not in allowed:
            _err(
                problems,
                "ROADMAP_UNKNOWN_KEY",
                where,
                "chiave sconosciuta `{}`. Ammesse: {}.".format(
                    key, ", ".join(sorted(allowed))
                ),
            )


def _as_list(value):
    if value is None:
        return []
    if isinstance(value, list):
        return value
    return [value]


def _positive_int(value, where, field, problems, default=None):
    if value is None:
        return default
    if isinstance(value, bool) or not isinstance(value, int):
        _err(
            problems,
            "ROADMAP_NOT_AN_INT",
            where,
            "{} deve essere un intero, trovato {!r}.".format(field, value),
        )
        return default
    if value < 0:
        _err(
            problems,
            "ROADMAP_NEGATIVE",
            where,
            "{} non puo' essere negativo ({}).".format(field, value),
        )
        return default
    return value


def _normalize_requires(raw, where, problems):
    """Accetta `- B1`, `- {item: B1}`, `- {item: B1, gate: VALIDATED}`."""
    out = []
    for entry in _as_list(raw):
        if isinstance(entry, str):
            out.append({"item": entry, "gate": "VALIDATED"})
            continue
        if not isinstance(entry, dict):
            _err(
                problems,
                "ROADMAP_BAD_REQUIRES",
                where,
                "dipendenza non riconosciuta: {!r}. Attesi `- ITEM` oppure "
                "`- {{item: ITEM, gate: VALIDATED}}`.".format(entry),
            )
            continue
        unknown = set(entry) - {"item", "gate"}
        if unknown:
            _err(
                problems,
                "ROADMAP_BAD_REQUIRES",
                where,
                "chiavi non ammesse nella dipendenza: {}.".format(
                    ", ".join(sorted(unknown))
                ),
            )
        item = entry.get("item")
        if not isinstance(item, str) or not item.strip():
            _err(
                problems,
                "ROADMAP_BAD_REQUIRES",
                where,
                "dipendenza senza `item`: {!r}.".format(entry),
            )
            continue
        gate = entry.get("gate", "VALIDATED")
        if gate not in GATES:
            _err(
                problems,
                "ROADMAP_BAD_GATE",
                where,
                "gate {!r} non ammesso. Ammessi: {}.".format(gate, ", ".join(GATES)),
            )
            gate = "VALIDATED"
        out.append({"item": item.strip(), "gate": gate})
    return out


def _default_resources(raw, problems):
    resources = raw if isinstance(raw, dict) else {}
    if raw is not None and not isinstance(raw, dict):
        _err(problems, "ROADMAP_BAD_RESOURCES", "resources", "`resources` deve essere un mapping.")

    workspaces = resources.get("workspaces") or {}
    if not isinstance(workspaces, dict):
        _err(
            problems,
            "ROADMAP_BAD_RESOURCES",
            "resources.workspaces",
            "`workspaces` deve essere un mapping workspaceGroup -> capacita'.",
        )
        workspaces = {}

    normalized_ws = {}
    for group, cfg in workspaces.items():
        where = "resources.workspaces.{}".format(group)
        if group not in WORKSPACE_GROUPS:
            _err(
                problems,
                "ROADMAP_BAD_WORKSPACE",
                where,
                "workspace group sconosciuto {!r}. Ammessi: {}.".format(
                    group, ", ".join(WORKSPACE_GROUPS)
                ),
            )
            continue
        cfg = cfg if isinstance(cfg, dict) else {}
        normalized_ws[group] = {
            "writerCapacity": _positive_int(
                cfg.get("writerCapacity"), where, "writerCapacity", problems, default=1
            )
        }
    for group in WORKSPACE_GROUPS:
        normalized_ws.setdefault(group, {"writerCapacity": 1})

    temp = resources.get("temporaryWorktrees") or {}
    temp = temp if isinstance(temp, dict) else {}
    unreal = resources.get("unrealEditor") or {}
    unreal = unreal if isinstance(unreal, dict) else {}

    return {
        "workspaces": normalized_ws,
        "temporaryWorktrees": {
            "capacity": _positive_int(
                temp.get("capacity"),
                "resources.temporaryWorktrees",
                "capacity",
                problems,
                default=0,
            )
        },
        "unrealEditor": {
            "leaseCapacity": _positive_int(
                unreal.get("leaseCapacity"),
                "resources.unrealEditor",
                "leaseCapacity",
                problems,
                default=1,
            )
        },
    }


def _default_wip(raw, problems):
    wip = raw if isinstance(raw, dict) else {}
    if raw is not None and not isinstance(raw, dict):
        _err(problems, "ROADMAP_BAD_WIP", "wip", "`wip` deve essere un mapping.")
    return {
        "perWorkspace": _positive_int(
            wip.get("perWorkspace"), "wip", "perWorkspace", problems, default=0
        ),
        "global": _positive_int(wip.get("global"), "wip", "global", problems, default=0),
    }


def normalize(document, source_path=None, content_hash=None):
    """Da documento grezzo a `(Roadmap | None, [Problem])`.

    Ritorna la roadmap anche in presenza di WARNING; ritorna None se c'e' almeno un
    ERROR, perche' una roadmap con un riferimento rotto pianificata «alla meglio»
    produrrebbe uno schedule che sembra valido.
    """
    problems = []

    if not isinstance(document, dict):
        _err(
            problems,
            "ROADMAP_NOT_A_MAPPING",
            "(radice)",
            "il documento non e' un mapping YAML.",
        )
        return None, problems

    _check_runtime_keys(document, "(radice)", problems)
    _check_unknown_keys(document, _ROOT_KEYS, "(radice)", problems)

    version = document.get("roadmapSchemaVersion")
    if version is None:
        _err(
            problems,
            "ROADMAP_SCHEMA_MISSING",
            "(radice)",
            "manca `roadmapSchemaVersion`. Senza, non e' possibile dire se questo "
            "rt3 sappia leggere il documento: atteso {}.".format(ROADMAP_SCHEMA_VERSION),
        )
    elif not isinstance(version, int) or isinstance(version, bool):
        _err(
            problems,
            "ROADMAP_SCHEMA_INVALID",
            "(radice)",
            "`roadmapSchemaVersion` deve essere un intero, trovato {!r}.".format(version),
        )
    elif version > ROADMAP_SCHEMA_VERSION:
        _err(
            problems,
            "ROADMAP_SCHEMA_TOO_NEW",
            "(radice)",
            "la roadmap dichiara schema v{}, questo rt3 legge fino a v{}. Aggiornare "
            "il workspace invece di abbassare il numero nel file.".format(
                version, ROADMAP_SCHEMA_VERSION
            ),
        )
    elif version < ROADMAP_SCHEMA_VERSION:
        _warn(
            problems,
            "ROADMAP_SCHEMA_OLD",
            "(radice)",
            "roadmap con schema v{}, corrente v{}: letta in compatibilita'.".format(
                version, ROADMAP_SCHEMA_VERSION
            ),
        )

    roadmap_id = document.get("id")
    if not isinstance(roadmap_id, str) or not roadmap_id.strip():
        _err(problems, "ROADMAP_ID_MISSING", "(radice)", "manca `id` della roadmap.")
        roadmap_id = None

    resources = _default_resources(document.get("resources"), problems)
    wip = _default_wip(document.get("wip"), problems)

    raw_epics = document.get("epics")
    if not isinstance(raw_epics, list) or not raw_epics:
        _err(
            problems,
            "ROADMAP_NO_EPICS",
            "(radice)",
            "`epics` deve essere una lista non vuota.",
        )
        raw_epics = []

    epics, items = [], {}
    short_index = {}  # id breve -> [chiavi canoniche]
    seen_epic_ids = set()

    for e_index, raw_epic in enumerate(raw_epics):
        where = "epics[{}]".format(e_index)
        if not isinstance(raw_epic, dict):
            _err(problems, "ROADMAP_BAD_EPIC", where, "l'epic non e' un mapping.")
            continue
        _check_runtime_keys(raw_epic, where, problems)
        _check_unknown_keys(raw_epic, _EPIC_KEYS, where, problems)

        epic_id = raw_epic.get("id")
        if not isinstance(epic_id, str) or not epic_id.strip():
            _err(problems, "ROADMAP_EPIC_ID_MISSING", where, "epic senza `id`.")
            continue
        epic_id = epic_id.strip()
        where = "epics[{}]({})".format(e_index, epic_id)
        if epic_id in seen_epic_ids:
            _err(
                problems,
                "ROADMAP_DUPLICATE_EPIC",
                where,
                "epic id duplicato: {}.".format(epic_id),
            )
            continue
        seen_epic_ids.add(epic_id)

        home_work = raw_epic.get("homeWork")
        if home_work not in WORKSPACE_GROUPS:
            _err(
                problems,
                "ROADMAP_BAD_HOMEWORK",
                where,
                "`homeWork` {!r} non ammesso. Ammessi: {}.".format(
                    home_work, ", ".join(WORKSPACE_GROUPS)
                ),
            )
            home_work = None

        raw_issues = raw_epic.get("issues")
        if not isinstance(raw_issues, list) or not raw_issues:
            _err(
                problems,
                "ROADMAP_EPIC_NO_ISSUES",
                where,
                "l'epic {} non ha issue.".format(epic_id),
            )
            raw_issues = []

        issue_keys = []
        for i_index, raw_issue in enumerate(raw_issues):
            i_where = "{}.issues[{}]".format(where, i_index)
            if not isinstance(raw_issue, dict):
                _err(problems, "ROADMAP_BAD_ISSUE", i_where, "la issue non e' un mapping.")
                continue
            _check_runtime_keys(raw_issue, i_where, problems)
            _check_unknown_keys(raw_issue, _ISSUE_KEYS, i_where, problems)

            issue_id = raw_issue.get("id")
            if not isinstance(issue_id, str) or not issue_id.strip():
                _err(problems, "ROADMAP_ISSUE_ID_MISSING", i_where, "issue senza `id`.")
                continue
            issue_id = issue_id.strip()
            key = "{}/{}".format(epic_id, issue_id)
            i_where = "{}({})".format(i_where, key)
            if key in items:
                _err(
                    problems,
                    "ROADMAP_DUPLICATE_ISSUE",
                    i_where,
                    "issue duplicata: {}.".format(key),
                )
                continue

            execution_work = raw_issue.get("executionWork") or home_work
            if execution_work is not None and execution_work not in WORKSPACE_GROUPS:
                _err(
                    problems,
                    "ROADMAP_BAD_EXECUTIONWORK",
                    i_where,
                    "`executionWork` {!r} non ammesso. Ammessi: {}.".format(
                        execution_work, ", ".join(WORKSPACE_GROUPS)
                    ),
                )
                execution_work = None

            estimate = raw_issue.get("estimate", 1)
            estimate = _positive_int(estimate, i_where, "estimate", problems, default=1)
            if estimate == 0:
                _warn(
                    problems,
                    "ROADMAP_ZERO_ESTIMATE",
                    i_where,
                    "estimate 0: la issue non contribuisce al critical path.",
                )

            resources_declared = []
            for res in _as_list(raw_issue.get("resources")):
                if res not in RESOURCE_KINDS:
                    _err(
                        problems,
                        "ROADMAP_UNKNOWN_RESOURCE",
                        i_where,
                        "risorsa sconosciuta {!r}. Ammesse: {}. Una risorsa non "
                        "riconosciuta verrebbe ignorata dallo scheduler.".format(
                            res, ", ".join(RESOURCE_KINDS)
                        ),
                    )
                    continue
                resources_declared.append(res)

            if "requiredGate" in raw_issue:
                _warn(
                    problems,
                    "ROADMAP_REQUIREDGATE_DEPRECATED",
                    i_where,
                    "`requiredGate` a livello di issue non ha effetto: il gate si "
                    "dichiara per dipendenza, in `requires[].gate`.",
                )

            issue = Issue(
                key=key,
                id=issue_id,
                epic_id=epic_id,
                title=raw_issue.get("title"),
                home_work=home_work,
                execution_work=execution_work,
                estimate=estimate,
                capabilities=[str(c) for c in _as_list(raw_issue.get("capabilities"))],
                write_set=[str(w) for w in _as_list(raw_issue.get("writeSet"))],
                resources=resources_declared,
                requires=_normalize_requires(raw_issue.get("requires"), i_where, problems),
                note=raw_issue.get("note"),
            )
            items[key] = issue
            issue_keys.append(key)
            short_index.setdefault(issue_id, []).append(key)

        epics.append(
            Epic(
                id=epic_id,
                title=raw_epic.get("title"),
                home_work=home_work,
                issue_keys=issue_keys,
                note=raw_epic.get("note"),
            )
        )

    # -- risoluzione dei riferimenti -------------------------------------
    for key, issue in items.items():
        resolved = []
        for dep in issue.requires:
            target = _resolve_ref(dep["item"], items, short_index, key, problems)
            if target is None:
                continue
            if target == key:
                _err(
                    problems,
                    "ROADMAP_SELF_DEPENDENCY",
                    key,
                    "la issue dipende da se stessa.",
                )
                continue
            resolved.append({"item": target, "gate": dep["gate"]})
        issue.requires = resolved

    if any(p.level == "ERROR" for p in problems):
        return None, problems

    roadmap = Roadmap(
        roadmap_id=roadmap_id,
        name=document.get("name") or roadmap_id,
        schema_version=version,
        epics=epics,
        items=items,
        resources=resources,
        wip=wip,
        source_path=source_path,
        content_hash=content_hash,
        note=document.get("note"),
    )
    return roadmap, problems


def _resolve_ref(ref, items, short_index, where, problems):
    """Da `B2` o `EPIC-B/B2` alla chiave canonica. None se irrisolvibile."""
    if ref in items:
        return ref
    candidates = short_index.get(ref, [])
    if len(candidates) == 1:
        return candidates[0]
    if len(candidates) > 1:
        _err(
            problems,
            "ROADMAP_AMBIGUOUS_REF",
            where,
            "il riferimento {!r} e' ambiguo: {}. Usare la forma EPIC/ISSUE.".format(
                ref, ", ".join(sorted(candidates))
            ),
        )
        return None
    _err(
        problems,
        "ROADMAP_UNKNOWN_REF",
        where,
        "dipendenza verso una issue inesistente: {!r}.".format(ref),
    )
    return None


# ---------------------------------------------------------------------------
# API
# ---------------------------------------------------------------------------


def content_hash(text):
    """Hash del sorgente. Identifica la VERSIONE di una roadmap fra i tre workspace.

    Normalizza le fini riga: lo stesso file, con `core.autocrlf=true`, ha byte diversi
    su Windows e su Linux, e un hash che ne dipendesse direbbe «roadmap diversa» a due
    workspace che hanno lo stesso commit.
    """
    normalized = text.replace("\r\n", "\n").replace("\r", "\n")
    return "sha256:" + hashlib.sha256(normalized.encode("utf-8")).hexdigest()[:16]


def load_document(path):
    """Legge e normalizza un file. Ritorna `(Roadmap | None, [Problem])`."""
    try:
        with open(path, "r", encoding="utf-8-sig") as handle:
            text = handle.read()
    except OSError as exc:
        raise RoadmapError("roadmap non leggibile ({}): {}".format(path, exc))

    try:
        document = parse_file(path)
    except YamlError as exc:
        raise RoadmapError(
            "roadmap non interpretabile ({}): {}".format(path, exc),
            problems=[Problem("ERROR", "ROADMAP_YAML_ERROR", path, str(exc))],
        )

    return normalize(document, source_path=path, content_hash=content_hash(text))


def load(path):
    """Carica una roadmap. Alza `RoadmapError` con tutti i problemi se non e' valida."""
    roadmap, problems = load_document(path)
    if roadmap is None:
        errors = [p for p in problems if p.level == "ERROR"]
        raise RoadmapError(
            "roadmap {} non valida: {} error{}.".format(
                path, len(errors), "e" if len(errors) == 1 else "i"
            ),
            problems=problems,
        )
    return roadmap, problems


def dumps(roadmap):
    """JSON stabile del PLAN normalizzato, come finisce nel database."""
    return json.dumps(roadmap.as_dict(), sort_keys=True, ensure_ascii=False)
