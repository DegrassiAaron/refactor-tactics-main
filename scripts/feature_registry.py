#!/usr/bin/env python3
"""Feature Registry: generazione, validazione e viste derivate.

Lo stato di una feature vive in `docs/roadmap/feature-registry.yaml` e in nessun altro posto.
Roadmap, Wiki e workbook referenziano il `feature_id`; questo script produce le viste derivate e
verifica che i riferimenti non siano rotti.

Nasce dallo stesso difetto che il repository ha gia' pagato quattro volte col conteggio dei test:
uno stato scritto a mano in due posti diverge, e la seconda copia diventa una bugia con la data
sbagliata. Qui la copia e' generata, e il generatore fallisce se la sorgente non regge.

Uso:
    python scripts/feature_registry.py validate           # gate: esce 1 se ci sono errori
    python scripts/feature_registry.py generate           # riscrive feature-registry.json
    python scripts/feature_registry.py wiki               # blocchi di stato + pagina Feature Status
    python scripts/feature_registry.py report             # tabella di audit (markdown su stdout)

Opzioni comuni:
    --wiki-root PATH    radice del clone della Wiki (deploy flat), per validare i `wiki:` refs
    --check             `generate`/`wiki` non scrivono: falliscono se l'output e' disallineato
"""
import argparse
import json
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REGISTRY_YAML = os.path.join(REPO, "docs", "roadmap", "feature-registry.yaml")
REGISTRY_JSON = os.path.join(REPO, "docs", "roadmap", "feature-registry.json")
ROADMAP_V01 = os.path.join(REPO, "docs", "roadmap", "roadmap-v0.1.md")
ROADMAP_CHECKPOINT = os.path.join(REPO, "docs", "roadmap", "roadmap-checkpoint.md")
TESTS_DIR = os.path.join(REPO, "Source", "RefactorTactics", "Tests")
SCENARIOS_DIR = os.path.join(REPO, "Scenarios")
WIKI_STATUS_PAGE = os.path.join(REPO, "docs", "wiki", "feature-status.md")

FEATURE_ID_RE = re.compile(r"^RT-FEAT-[A-Z0-9]+(-[A-Z0-9]+)*$")
GATE_NAMES = [
    "spec", "data", "runtime", "log_debug", "automation",
    "scenario", "ui_wiki", "packaged", "network_privacy",
]
GATE_VALUES = {"done", "partial", "todo", "na"}
RELEASES = {"v0.1", "v0.2", "future"}
PRIORITIES = {"P0", "P1", "P2", "P3"}
KINDS = {"gameplay", "ui", "tooling", "data", "infra", "content"}

# Ordine di maturita'. `DEFERRED` e `BLOCKED` sono fuori scala: dichiarano una decisione,
# non un grado di completezza, e per questo il controllo di coerenza li salta.
STATUS_ORDER = [
    "IDEA", "DESIGNED", "SPECIFIED", "IMPLEMENTING",
    "TESTABLE", "INTEGRATED", "RELEASE_READY", "DONE",
]
STATUS_OFF_SCALE = {"DEFERRED", "BLOCKED"}
ALL_STATUS = set(STATUS_ORDER) | STATUS_OFF_SCALE

# Un gate `na` conta come soddisfatto: «non applicabile» e' una risposta, non un buco.
# `partial` non lo e' mai: e' il modo in cui una feature dichiara di non aver finito.
SATISFIED = {"done", "na"}

MARKER_BEGIN = "<!-- RT_FEATURE_STATUS:BEGIN {fid} -->"
MARKER_END = "<!-- RT_FEATURE_STATUS:END {fid} -->"
MARKER_ANY = re.compile(
    r"<!-- RT_FEATURE_STATUS:BEGIN (?P<fid>[A-Za-z0-9\-]+) -->.*?"
    r"<!-- RT_FEATURE_STATUS:END (?P=fid) -->",
    re.S,
)


# ---------------------------------------------------------------------------
# Lettura della sorgente e delle evidenze
# ---------------------------------------------------------------------------

def load_registry():
    try:
        import yaml
    except ImportError:
        sys.exit("Serve PyYAML: pip install pyyaml")
    with open(REGISTRY_YAML, encoding="utf-8") as fh:
        return yaml.safe_load(fh)


def known_tests():
    """I nomi dei test sono la prova di cio' che esiste: si misurano, non si citano."""
    names = set()
    if not os.path.isdir(TESTS_DIR):
        return names
    pattern = re.compile(r'"(RefactorTactics\.[A-Za-z0-9_.]+)"')
    for entry in sorted(os.listdir(TESTS_DIR)):
        if not entry.endswith(".cpp"):
            continue
        with open(os.path.join(TESTS_DIR, entry), encoding="utf-8", errors="replace") as fh:
            names.update(pattern.findall(fh.read()))
    return names


def known_scenarios():
    """ScenarioId dichiarati nei file, non dedotti dal percorso (scenario-index-e-tag.md §3)."""
    ids = set()
    for root, _dirs, files in os.walk(SCENARIOS_DIR):
        for name in files:
            if not name.endswith(".json") or name.startswith("_"):
                continue
            path = os.path.join(root, name)
            try:
                with open(path, encoding="utf-8-sig") as fh:
                    data = json.load(fh)
            except (json.JSONDecodeError, OSError):
                continue
            sid = data.get("scenarioId")
            if sid:
                ids.add(sid)
    return ids


def known_roadmap_refs():
    """Epic e checkpoint dichiarati dalla roadmap di release; milestone da quella di esecuzione."""
    epics, checkpoints, milestones = set(), set(), set()
    if os.path.isfile(ROADMAP_V01):
        text = open(ROADMAP_V01, encoding="utf-8").read()
        epics.update(re.findall(r"^### (E\d+) —", text, re.M))
        epics.update(re.findall(r"\*\*(E\d+)\*\*", text))
        checkpoints.update(re.findall(r"CP (\d+\.\d+)", text))
        checkpoints.update(re.findall(r"^\| \*\*(\d+\.\d+)\*\*", text, re.M))
    if os.path.isfile(ROADMAP_CHECKPOINT):
        text = open(ROADMAP_CHECKPOINT, encoding="utf-8").read()
        milestones.update(re.findall(r"\*\*(M\d+)\*\*", text))
    return epics, checkpoints, milestones


def test_pattern_matches(pattern, names):
    """`Actions.Wait` vale per tutto il sottoalbero `Actions.Wait.*`.

    I nomi dei test hanno profondita' variabile e la regola del progetto vieta che un nome sia
    prefisso di un altro (§6.1 della roadmap): un riferimento a un ramo non e' quindi ambiguo,
    e obbligare a scrivere `.*` ovunque renderebbe il registry rumoroso senza guadagno.
    """
    if pattern.endswith("*"):
        prefix = pattern[:-1]
        return any(n.startswith(prefix) for n in names)
    return pattern in names or any(n.startswith(pattern + ".") for n in names)


def scenario_lists(feature):
    """`scenarios` accetta una lista piatta oppure {planned: [...]} per cio' che non esiste ancora."""
    raw = feature.get("scenarios") or []
    if isinstance(raw, dict):
        return list(raw.get("present", []) or []), list(raw.get("planned", []) or [])
    present, planned = [], []
    for item in raw:
        if isinstance(item, dict) and "planned" in item:
            planned.extend(item["planned"] or [])
        else:
            present.append(item)
    return present, planned


# ---------------------------------------------------------------------------
# Derivazione dello stato dai gate
# ---------------------------------------------------------------------------

def derive_status(gates):
    """Lo stato non e' un'opinione: e' una funzione dei gate.

    Restituisce il gradino piu' alto che i gate reggono. Il validator confronta questo valore
    con quello dichiarato: se il dichiarato e' piu' alto, e' un errore — e' esattamente il modo
    in cui una feature finisce per essere «Done» perche' si vede in PIE.
    """
    def ok(*names):
        return all(gates.get(n) in SATISFIED for n in names)

    core = ("spec", "data", "runtime", "log_debug", "automation")
    if ok(*core, "scenario", "ui_wiki", "packaged", "network_privacy"):
        return "DONE"
    if ok(*core, "scenario", "ui_wiki"):
        return "RELEASE_READY"
    if ok(*core, "scenario"):
        return "INTEGRATED"
    if ok("spec", "runtime", "automation"):
        return "TESTABLE"
    if gates.get("runtime") in ("done", "partial"):
        return "IMPLEMENTING"
    if gates.get("spec") == "done":
        return "SPECIFIED"
    if gates.get("spec") == "partial":
        return "DESIGNED"
    return "IDEA"


def gate_progress(gates):
    """«6/8 gate» invece di «73%»: il primo si verifica, il secondo si contratta."""
    applicable = [g for g in GATE_NAMES if gates.get(g) != "na"]
    done = [g for g in applicable if gates.get(g) == "done"]
    return len(done), len(applicable)


# ---------------------------------------------------------------------------
# Validazione
# ---------------------------------------------------------------------------

def validate(registry, wiki_root=None):
    errors, warnings = [], []
    features = registry.get("features") or []
    tests = known_tests()
    scenarios = known_scenarios()
    epics, checkpoints, milestones = known_roadmap_refs()
    ids = [f.get("feature_id") for f in features]

    for fid in sorted({i for i in ids if ids.count(i) > 1}):
        errors.append(f"FeatureId duplicato: {fid}")

    id_set = set(ids)
    testable_or_more = set(STATUS_ORDER[STATUS_ORDER.index("TESTABLE"):])

    for feature in features:
        fid = feature.get("feature_id", "<senza id>")
        where = f"[{fid}]"

        if not FEATURE_ID_RE.match(fid or ""):
            errors.append(f"{where} feature_id non conforme a RT-FEAT-<AREA>-<NOME>")

        status = feature.get("status")
        if status not in ALL_STATUS:
            errors.append(f"{where} status non valido: {status!r}")
        if feature.get("release") not in RELEASES:
            errors.append(f"{where} release non valida: {feature.get('release')!r}")
        if feature.get("priority") not in PRIORITIES:
            errors.append(f"{where} priority non valida: {feature.get('priority')!r}")
        if feature.get("kind") not in KINDS:
            errors.append(f"{where} kind non valido: {feature.get('kind')!r}")

        gates = feature.get("gates") or {}
        for gate in GATE_NAMES:
            value = gates.get(gate)
            if value is None:
                errors.append(f"{where} gate mancante: {gate}")
            elif value not in GATE_VALUES:
                errors.append(f"{where} gate {gate} non valido: {value!r}")
        for gate in gates:
            if gate not in GATE_NAMES:
                errors.append(f"{where} gate sconosciuto: {gate}")

        if status in STATUS_ORDER and all(gates.get(g) in GATE_VALUES for g in GATE_NAMES):
            derived = derive_status(gates)
            if status != derived:
                missing = [g for g in GATE_NAMES if gates.get(g) not in SATISFIED]
                if STATUS_ORDER.index(status) > STATUS_ORDER.index(derived):
                    detail = f"gate non completati: {', '.join(missing)}"
                else:
                    detail = "i gate dicono che e' piu' avanti di quanto dichiari"
                errors.append(
                    f"{where} status {status} diverge dai gate (derivato: {derived}); {detail}"
                )

        last = feature.get("last_verified") or {}
        if status in testable_or_more and not (last.get("date") and last.get("commit")):
            errors.append(f"{where} last_verified assente ma status e' {status}")

        roadmap = feature.get("roadmap") or {}
        epic = roadmap.get("epic")
        if epic and epic not in epics:
            errors.append(f"{where} epic inesistente nella roadmap di release: {epic}")
        milestone = roadmap.get("milestone")
        if milestone and milestone not in milestones:
            errors.append(f"{where} milestone inesistente nella roadmap di esecuzione: {milestone}")
        for cp in roadmap.get("checkpoints") or []:
            if str(cp) not in checkpoints:
                errors.append(f"{where} checkpoint inesistente: CP {cp}")

        for dep in feature.get("dependencies") or []:
            if dep not in id_set:
                errors.append(f"{where} dipendenza verso FeatureId inesistente: {dep}")

        for spec in feature.get("owner_specs") or []:
            if not os.path.isfile(os.path.join(REPO, spec)):
                errors.append(f"{where} owner spec inesistente: {spec}")

        for pattern in feature.get("tests") or []:
            if not test_pattern_matches(pattern, tests):
                errors.append(f"{where} test ref senza corrispondenza nella suite: {pattern}")

        present, planned = scenario_lists(feature)
        for sid in present:
            if sid not in scenarios:
                errors.append(f"{where} ScenarioId inesistente: {sid}")
        for sid in planned:
            if sid in scenarios:
                warnings.append(f"{where} scenario dichiarato planned ma ora esiste: {sid} — promuovilo")

        for ref in feature.get("wiki_refs") or []:
            if ref.startswith("wiki:"):
                if wiki_root:
                    page = os.path.join(wiki_root, ref[len("wiki:"):] + ".md")
                    if not os.path.isfile(page):
                        errors.append(f"{where} pagina Wiki inesistente nel clone: {ref}")
            elif not os.path.isfile(os.path.join(REPO, ref)):
                errors.append(f"{where} pagina Wiki sorgente inesistente: {ref}")

        # --- warning: lacune che non rompono nulla ma vanno viste ---
        if not (feature.get("wiki_refs") or []):
            warnings.append(f"{where} nessuna pagina Wiki collegata")
        if status in testable_or_more and feature.get("kind") == "gameplay" and not present:
            warnings.append(f"{where} feature di gameplay {status} senza scenario che la dimostri")
        if status == "SPECIFIED" and not (feature.get("issues") or []) and not epic and not milestone:
            warnings.append(f"{where} SPECIFIED senza issue ne' assegnazione in roadmap")
        if planned:
            warnings.append(
                f"{where} {len(planned)} scenario/i dichiarati planned: "
                + ", ".join(planned)
            )

    # --- riferimenti a feature dalle pagine gia' generate ---
    for page, fids in wiki_blocks_in_repo().items():
        for fid in fids:
            if fid not in id_set:
                errors.append(f"[{page}] blocco di stato per FeatureId inesistente: {fid}")

    return errors, warnings


def wiki_blocks_in_repo():
    """Pagine del repo che contengono gia' un blocco generato, con i FeatureId citati."""
    found = {}
    for base in ("docs/wiki", "docs/characters"):
        root_dir = os.path.join(REPO, base)
        for root, _dirs, files in os.walk(root_dir):
            for name in files:
                if not name.endswith(".md"):
                    continue
                path = os.path.join(root, name)
                text = open(path, encoding="utf-8", errors="replace").read()
                fids = MARKER_ANY.findall(text)
                fids = re.findall(r"RT_FEATURE_STATUS:BEGIN ([A-Za-z0-9\-]+)", text)
                if fids:
                    found[os.path.relpath(path, REPO).replace("\\", "/")] = fids
    return found


# ---------------------------------------------------------------------------
# Viste derivate
# ---------------------------------------------------------------------------

def jsonable(value):
    """PyYAML restituisce `datetime.date` per `2026-08-08`: nel JSON serve la stringa ISO."""
    import datetime
    if isinstance(value, (datetime.date, datetime.datetime)):
        return value.isoformat()
    if isinstance(value, dict):
        return {k: jsonable(v) for k, v in value.items()}
    if isinstance(value, list):
        return [jsonable(v) for v in value]
    return value


def build_json(registry):
    features = []
    for feature in registry.get("features") or []:
        gates = feature.get("gates") or {}
        done, total = gate_progress(gates)
        present, planned = scenario_lists(feature)
        roadmap = feature.get("roadmap") or {}
        features.append({
            "feature_id": feature.get("feature_id"),
            "title": feature.get("title"),
            "area": feature.get("area"),
            "kind": feature.get("kind"),
            "release": feature.get("release"),
            "priority": feature.get("priority"),
            "status": feature.get("status"),
            "status_derived": derive_status(gates) if all(
                gates.get(g) in GATE_VALUES for g in GATE_NAMES) else None,
            "gates": {g: gates.get(g) for g in GATE_NAMES},
            "gates_done": done,
            "gates_applicable": total,
            "roadmap": {
                "epic": roadmap.get("epic"),
                "milestone": roadmap.get("milestone"),
                "checkpoints": [str(c) for c in (roadmap.get("checkpoints") or [])],
            },
            "dependencies": feature.get("dependencies") or [],
            "owner_specs": feature.get("owner_specs") or [],
            "issues": feature.get("issues") or [],
            "tests": feature.get("tests") or [],
            "scenarios": present,
            "scenarios_planned": planned,
            "wiki_refs": feature.get("wiki_refs") or [],
            "wiki_note": (feature.get("wiki_note") or "").strip(),
            "last_verified": jsonable(feature.get("last_verified") or {}),
            "notes": (feature.get("notes") or "").strip(),
        })
    return {
        "_generated": "NON EDITARE: generato da docs/roadmap/feature-registry.yaml "
                      "con scripts/feature_registry.py generate",
        "meta": jsonable(registry.get("meta") or {}),
        "count": len(features),
        "features": features,
    }


def status_block(entry):
    roadmap = entry["roadmap"]
    ref = roadmap.get("epic") or roadmap.get("milestone") or "—"
    if roadmap.get("checkpoints"):
        ref += " · CP " + ", ".join(roadmap["checkpoints"])
    scenario = entry["scenarios"][0] if entry["scenarios"] else (
        entry["scenarios_planned"][0] + " (pianificato)" if entry["scenarios_planned"] else "—")
    verified = entry["last_verified"] or {}
    lines = [
        MARKER_BEGIN.format(fid=entry["feature_id"]),
        "",
        "> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  ",
        f"> Feature: `{entry['feature_id']}` · Release: `{entry['release']}` · Roadmap: `{ref}`  ",
        f"> Stato: **{entry['status']}** · Gate: `{entry['gates_done']}/{entry['gates_applicable']}`  ",
        f"> Scenario: `{scenario}`  ",
    ]
    # La nota e' l'unico testo libero del blocco: serve a non perdere il dettaglio che i banner
    # scritti a mano portavano («esiste il dato ma nessun eroe lo usa»), tenendolo pero' in un
    # posto solo. Resta una riga: se serve un paragrafo, va nel corpo della pagina.
    if entry.get("wiki_note"):
        lines.append(f"> {entry['wiki_note']}  ")
    lines += [
        f"> Verificato il `{verified.get('date', '—')}` su `{verified.get('commit', '—')}`",
        "",
        MARKER_END.format(fid=entry["feature_id"]),
    ]
    return "\n".join(lines)


def render_status_page(data):
    by_release = {"v0.1": [], "v0.2": [], "future": []}
    for entry in data["features"]:
        by_release.setdefault(entry["release"], []).append(entry)

    out = [
        "# Stato delle feature",
        "",
        "> `GENERATO` · Vista del **Feature Registry** "
        "(`docs/roadmap/feature-registry.yaml` nel repository del gioco).",
        "> Non modificare a mano: si rigenera con "
        "`python scripts/feature_registry.py wiki`.",
        f"> Feature tracciate: **{data['count']}** · "
        f"Ultimo audit completo: **{(data['meta'].get('last_full_audit') or {}).get('date', '—')}** "
        f"su `{(data['meta'].get('last_full_audit') or {}).get('commit', '—')}`.",
        "",
        "Questa pagina dice **cosa esiste davvero**. Una meccanica descritta altrove nella Wiki e",
        "marcata qui come `SPECIFIED` o `DESIGNED` e' progettata, non giocabile: la Wiki racconta",
        "il gioco che sara', questa tabella dice a che punto e'.",
        "",
        "## Cosa significano gli stati",
        "",
        "| Stato | Significato |",
        "|---|---|",
        "| `IDEA` | Nominata, nessun documento che la definisca |",
        "| `DESIGNED` | Un brief la descrive, le regole non sono chiuse |",
        "| `SPECIFIED` | Regole decise e documentate, nessun codice |",
        "| `IMPLEMENTING` | Codice presente ma incompleto o non coperto da test |",
        "| `TESTABLE` | Implementata e coperta da test automatici |",
        "| `INTEGRATED` | Testata **e** dimostrata da uno scenario giocabile |",
        "| `RELEASE_READY` | Anche UI e documentazione allineate |",
        "| `DONE` | Verificata anche su build packaged |",
        "",
        "«Gate» conta i controlli superati sui controlli applicabili: `6/8` si verifica,",
        "«73%» no.",
        "",
    ]

    titles = {"v0.1": "Release v0.1", "v0.2": "Release v0.2", "future": "Oltre la v0.2"}
    for release in ("v0.1", "v0.2", "future"):
        entries = by_release.get(release) or []
        if not entries:
            continue
        out += [f"## {titles.get(release, release)}", ""]
        for area in sorted({e["area"] for e in entries}):
            out += [
                f"### {area}",
                "",
                "| Feature | Titolo | Roadmap | Stato | Gate | Scenario |",
                "|---|---|---|---|---:|---|",
            ]
            for entry in sorted(entries, key=lambda e: e["feature_id"]):
                if entry["area"] != area:
                    continue
                roadmap = entry["roadmap"]
                ref = roadmap.get("epic") or roadmap.get("milestone") or "—"
                if roadmap.get("checkpoints"):
                    ref += " · " + ", ".join("CP " + c for c in roadmap["checkpoints"])
                if entry["scenarios"]:
                    scenario = "`" + "` · `".join(entry["scenarios"][:2]) + "`"
                elif entry["scenarios_planned"]:
                    scenario = "_pianificato_"
                else:
                    scenario = "—"
                out.append(
                    f"| `{entry['feature_id']}` | {entry['title']} | {ref} | "
                    f"**{entry['status']}** | {entry['gates_done']}/{entry['gates_applicable']} | "
                    f"{scenario} |"
                )
            out.append("")
    return "\n".join(out).rstrip() + "\n"


def apply_wiki_blocks(data, check=False):
    """Inserisce o aggiorna il blocco di stato nelle pagine referenziate dal registry."""
    by_page = {}
    for entry in data["features"]:
        for ref in entry["wiki_refs"]:
            if ref.startswith("wiki:"):
                continue
            by_page.setdefault(ref, []).append(entry)

    changed, stale = [], []
    for ref, entries in sorted(by_page.items()):
        path = os.path.join(REPO, ref)
        if not os.path.isfile(path):
            continue
        original = open(path, encoding="utf-8").read()
        text = original
        wanted = {e["feature_id"]: status_block(e) for e in entries}

        existing = set(re.findall(r"RT_FEATURE_STATUS:BEGIN ([A-Za-z0-9\-]+)", text))
        for fid in existing - set(wanted):
            stale.append(f"{ref}: blocco per {fid} non e' piu' referenziato dal registry")

        # I blocchi nuovi si inseriscono sempre in cima: iterando al contrario, l'ordine finale
        # sulla pagina e' quello del registry invece del suo rovescio.
        for fid, block in reversed(list(wanted.items())):
            pattern = re.compile(
                re.escape(MARKER_BEGIN.format(fid=fid)) + r".*?" + re.escape(MARKER_END.format(fid=fid)),
                re.S,
            )
            if pattern.search(text):
                text = pattern.sub(lambda _m, b=block: b, text)
            else:
                text = insert_block(text, block)

        if text != original:
            changed.append(ref)
            if not check:
                with open(path, "w", encoding="utf-8", newline="\n") as fh:
                    fh.write(text)
    return changed, stale


def deploy_name(source_ref):
    """Nome della pagina nel clone della Wiki, che e' un repo separato con file **flat**.

    La convenzione e' quella gia' in uso nel clone (`DEPLOY.md`): le cartelle della sorgente
    diventano un prefisso nel nome, perche' la GitHub Wiki non ha gerarchia.
    """
    ref = source_ref.replace("\\", "/")
    stem = os.path.splitext(os.path.basename(ref))[0]
    if ref.startswith("docs/wiki/game/"):
        return stem + ".md"
    if ref.startswith("docs/wiki/meccaniche/"):
        return "Meccanica-" + stem + ".md"
    if ref.startswith("docs/wiki/fazioni/"):
        if stem == "index":
            return "Fazioni.md"
        return "Fazione-" + "-".join(p.capitalize() for p in stem.split("-")) + ".md"
    if ref.startswith("docs/characters/v0."):
        return "Personaggio-" + stem + ".md"
    if ref == "docs/characters/index.md":
        return "Personaggi.md"
    if ref == "docs/wiki/feature-status.md":
        return "Stato-delle-feature.md"
    return None


def apply_wiki_deploy(data, wiki_root, check=True):
    """Porta i blocchi generati nel clone della Wiki. Di norma si esegue con `--check`.

    Il clone e' un **altro repository**, pubblico, che altre sessioni possono avere in lavorazione:
    scriverci e' un'azione che si chiede, non si assume.
    """
    by_page = {}
    for entry in data["features"]:
        for ref in entry["wiki_refs"]:
            if ref.startswith("wiki:"):
                continue
            by_page.setdefault(ref, []).append(entry)

    changed, missing = [], []
    for ref, entries in sorted(by_page.items()):
        name = deploy_name(ref)
        if not name:
            missing.append(f"{ref}: nessuna pagina di deploy corrispondente")
            continue
        path = os.path.join(wiki_root, name)
        if not os.path.isfile(path):
            missing.append(f"{ref} -> {name}: la pagina non esiste nel clone")
            continue
        original = open(path, encoding="utf-8").read()
        text = original
        for entry in reversed(entries):
            fid = entry["feature_id"]
            block = status_block(entry)
            pattern = re.compile(
                re.escape(MARKER_BEGIN.format(fid=fid)) + r".*?" + re.escape(MARKER_END.format(fid=fid)),
                re.S,
            )
            text = pattern.sub(lambda _m, b=block: b, text) if pattern.search(text) else insert_block(text, block)
        if text != original:
            changed.append(name)
            if not check:
                with open(path, "w", encoding="utf-8", newline="\n") as fh:
                    fh.write(text)

    status_source = os.path.join(REPO, "docs", "wiki", "feature-status.md")
    if os.path.isfile(status_source):
        target = os.path.join(wiki_root, "Stato-delle-feature.md")
        content = open(status_source, encoding="utf-8").read()
        current = open(target, encoding="utf-8").read() if os.path.isfile(target) else None
        if current != content:
            changed.append("Stato-delle-feature.md")
            if not check:
                with open(target, "w", encoding="utf-8", newline="\n") as fh:
                    fh.write(content)
    return changed, missing


def insert_block(text, block):
    """Il blocco va dopo il titolo e l'eventuale citazione introduttiva, prima del corpo."""
    lines = text.split("\n")
    index = 0
    for i, line in enumerate(lines):
        if line.startswith("# "):
            index = i + 1
            break
    while index < len(lines) and (lines[index].strip() == "" or lines[index].startswith(">")):
        index += 1
    prefix = lines[:index]
    suffix = lines[index:]
    if prefix and prefix[-1].strip() != "":
        prefix.append("")
    return "\n".join(prefix + [block, ""] + suffix)


ROADMAP_MARKER_BEGIN = "<!-- RT_FEATURE_BY_EPIC:BEGIN -->"
ROADMAP_MARKER_END = "<!-- RT_FEATURE_BY_EPIC:END -->"


def render_features_by_epic(data):
    """Mappa epic → feature per la roadmap di release: generata, non ricopiata a mano."""
    by_epic = {}
    unassigned = []
    for entry in data["features"]:
        epic = entry["roadmap"].get("epic")
        if epic:
            by_epic.setdefault(epic, []).append(entry)
        elif entry["release"] == "v0.1":
            unassigned.append(entry)

    lines = [
        ROADMAP_MARKER_BEGIN,
        "",
        "| Epic | Feature | Stato | Gate |",
        "|---|---|---|---:|",
    ]
    for epic in sorted(by_epic, key=lambda e: int(e[1:])):
        entries = sorted(by_epic[epic], key=lambda e: e["feature_id"])
        for i, entry in enumerate(entries):
            label = f"**{epic}**" if i == 0 else ""
            lines.append(
                f"| {label} | `{entry['feature_id']}` — {entry['title']} | "
                f"{entry['status']} | {entry['gates_done']}/{entry['gates_applicable']} |"
            )
    lines.append("")
    if unassigned:
        lines += [
            "**Feature v0.1 senza epic** — lavoro dentro lo scope della release che nessuna delle 20 epic",
            "copre. Non e' una svista del registry: e' un buco della roadmap, ed e' il motivo per cui questa",
            "tabella e' generata.",
            "",
            "| Feature | Vista | Stato | Gate |",
            "|---|---|---|---:|",
        ]
        for entry in sorted(unassigned, key=lambda e: e["feature_id"]):
            where = entry["roadmap"].get("milestone") or "—"
            lines.append(
                f"| `{entry['feature_id']}` — {entry['title']} | {where} | "
                f"{entry['status']} | {entry['gates_done']}/{entry['gates_applicable']} |"
            )
        lines.append("")
    lines.append(ROADMAP_MARKER_END)
    return "\n".join(lines)


def apply_roadmap_block(data, check=False):
    if not os.path.isfile(ROADMAP_V01):
        return False
    original = open(ROADMAP_V01, encoding="utf-8").read()
    if ROADMAP_MARKER_BEGIN not in original:
        return False
    pattern = re.compile(
        re.escape(ROADMAP_MARKER_BEGIN) + r".*?" + re.escape(ROADMAP_MARKER_END), re.S)
    block = render_features_by_epic(data)
    text = pattern.sub(lambda _m: block, original)
    if text == original:
        return False
    if not check:
        with open(ROADMAP_V01, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text)
    return True


CHARACTERS_WORKBOOK = os.path.join(
    REPO, "docs", "characters", "data", "RefactorTactics_Characters_Wiki_Data_v0.4.xlsx")
REFS_SHEET = "15_Wiki_Feature_Refs"


def wiki_entity_id(ref):
    """Entita' della Wiki a cui una pagina corrisponde, per il workbook."""
    ref = ref.replace("\\", "/")
    stem = os.path.splitext(os.path.basename(ref))[0]
    if ref.startswith("docs/characters/v0.") and stem != "index":
        return "Hero." + stem.capitalize()
    if ref.startswith("docs/wiki/fazioni/") and stem != "index":
        return "Faction." + "".join(p.capitalize() for p in stem.split("-"))
    if ref.startswith("docs/wiki/meccaniche/"):
        return "Mechanic." + "".join(p.capitalize() for p in stem.split("-"))
    if ref.startswith("docs/wiki/game/"):
        return "Guide." + "".join(p.capitalize() for p in stem.split("-"))
    return None


def feature_refs_rows(data):
    """Relazioni entita' Wiki → FeatureId, derivate dal registry. Nessuno stato: solo riferimenti."""
    rows = []
    for entry in data["features"]:
        for ref in entry["wiki_refs"]:
            entity = wiki_entity_id(ref)
            if not entity:
                continue
            if entity.startswith("Faction."):
                relation = "belongs-to"
            elif entry["area"] == "Characters":
                relation = "release"
            elif entity.startswith("Hero."):
                relation = "demonstrates"
            else:
                relation = "documents"
            rows.append((entity, entry["feature_id"], relation, ref))
        for sid in entry["scenarios_planned"]:
            if sid.startswith("Team."):
                rows.append(("Scenario." + sid, entry["feature_id"], "validates", "pianificato"))
    return sorted(set(rows))


def apply_workbook(data, check=False):
    """Riscrive la sheet delle relazioni nel workbook di character authoring.

    Il workbook **non** tiene lo stato: quello vive nel registry. Qui stanno solo i riferimenti,
    perche' una riga «Design_Status = IMPLEMENTED» scritta a mano e' la terza copia dello stesso
    fatto, e la terza copia e' quella che nessuno ricorda di aggiornare.
    """
    try:
        import openpyxl
    except ImportError:
        return None, "serve openpyxl: pip install openpyxl"
    if not os.path.isfile(CHARACTERS_WORKBOOK):
        return None, f"workbook non trovato: {CHARACTERS_WORKBOOK}"

    rows = feature_refs_rows(data)
    workbook = openpyxl.load_workbook(CHARACTERS_WORKBOOK)
    existing = []
    if REFS_SHEET in workbook.sheetnames:
        sheet = workbook[REFS_SHEET]
        for row in sheet.iter_rows(min_row=4, values_only=True):
            if row and row[0]:
                existing.append(tuple(str(c) if c is not None else "" for c in row[:4]))
    if existing == [tuple(r) for r in rows]:
        return [], None
    if check:
        return rows, None

    if REFS_SHEET in workbook.sheetnames:
        del workbook[REFS_SHEET]
    sheet = workbook.create_sheet(REFS_SHEET)
    sheet["A1"] = ("Riferimenti Wiki -> Feature Registry - GENERATO, non modificare a mano "
                   "(python scripts/feature_registry.py workbook)")
    sheet["A2"] = ("Lo stato di una feature NON vive qui: sta in docs/roadmap/feature-registry.yaml "
                   "ed e' derivato dai gate. Qui ci sono solo i riferimenti.")
    for column, header in enumerate(("WikiEntityId", "FeatureId", "Relation", "Source"), start=1):
        sheet.cell(row=3, column=column, value=header)
    for index, row in enumerate(rows, start=4):
        for column, value in enumerate(row, start=1):
            sheet.cell(row=index, column=column, value=value)
    for column, width in zip("ABCD", (34, 38, 14, 46)):
        sheet.column_dimensions[column].width = width
    workbook.save(CHARACTERS_WORKBOOK)
    return rows, None


def render_audit(data):
    out = [
        "| FeatureId | Titolo | Release | Stato | Roadmap | Owner spec | Test | Scenari | Wiki |",
        "|---|---|---|---|---|---:|---:|---:|---:|",
    ]
    for entry in sorted(data["features"], key=lambda e: (e["release"], e["area"], e["feature_id"])):
        roadmap = entry["roadmap"]
        ref = roadmap.get("epic") or roadmap.get("milestone") or "—"
        out.append(
            f"| `{entry['feature_id']}` | {entry['title']} | {entry['release']} | "
            f"{entry['status']} ({entry['gates_done']}/{entry['gates_applicable']}) | {ref} | "
            f"{len(entry['owner_specs'])} | {len(entry['tests'])} | "
            f"{len(entry['scenarios'])} | {len(entry['wiki_refs'])} |"
        )
    return "\n".join(out)


# ---------------------------------------------------------------------------

def write_if_needed(path, content, check):
    current = open(path, encoding="utf-8").read() if os.path.isfile(path) else None
    if current == content:
        return False
    if not check:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(content)
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("command",
                        choices=["validate", "generate", "wiki", "workbook", "deploy", "report"])
    parser.add_argument("--wiki-root", help="radice del clone della Wiki (deploy flat)")
    parser.add_argument("--check", action="store_true", help="non scrivere: fallisci se disallineato")
    parser.add_argument("--write", action="store_true",
                        help="`deploy`: scrive davvero nel clone della Wiki (default: sola lettura)")
    args = parser.parse_args()

    registry = load_registry()
    data = build_json(registry)

    if args.command == "validate":
        errors, warnings = validate(registry, args.wiki_root)
        print(f"Feature: {data['count']}")
        print(f"  riferimenti wiki   : {sum(len(f['wiki_refs']) for f in data['features'])}")
        print(f"  riferimenti roadmap: "
              f"{sum(1 for f in data['features'] if f['roadmap']['epic'] or f['roadmap']['milestone'])}")
        print(f"  riferimenti scenari: {sum(len(f['scenarios']) for f in data['features'])} "
              f"(+{sum(len(f['scenarios_planned']) for f in data['features'])} pianificati)")
        print(f"  riferimenti test   : {sum(len(f['tests']) for f in data['features'])}")
        for warning in warnings:
            print(f"WARN  {warning}")
        for error in errors:
            print(f"ERROR {error}")
        print(f"\nerrori: {len(errors)} · warning: {len(warnings)}")
        return 1 if errors else 0

    if args.command == "generate":
        content = json.dumps(data, ensure_ascii=False, indent=2) + "\n"
        if write_if_needed(REGISTRY_JSON, content, args.check):
            if args.check:
                print("feature-registry.json non e' allineato alla sorgente YAML")
                return 1
            print(f"scritto {os.path.relpath(REGISTRY_JSON, REPO)}")
        else:
            print("feature-registry.json gia' allineato")
        return 0

    if args.command == "wiki":
        changed, stale = apply_wiki_blocks(data, args.check)
        page_changed = write_if_needed(WIKI_STATUS_PAGE, render_status_page(data), args.check)
        roadmap_changed = apply_roadmap_block(data, args.check)
        for note in stale:
            print(f"WARN  {note}")
        if args.check and (changed or page_changed or roadmap_changed):
            for ref in changed:
                print(f"disallineato: {ref}")
            if page_changed:
                print(f"disallineato: {os.path.relpath(WIKI_STATUS_PAGE, REPO)}")
            if roadmap_changed:
                print(f"disallineato: {os.path.relpath(ROADMAP_V01, REPO)}")
            return 1
        for ref in changed:
            print(f"aggiornato {ref}")
        if page_changed:
            print(f"aggiornato {os.path.relpath(WIKI_STATUS_PAGE, REPO)}")
        if roadmap_changed:
            print(f"aggiornato {os.path.relpath(ROADMAP_V01, REPO)}")
        if not (changed or page_changed or roadmap_changed):
            print("pagine gia' allineate")
        return 0

    if args.command == "workbook":
        rows, error = apply_workbook(data, args.check)
        if error:
            print(error)
            return 1
        if not rows:
            print(f"{REFS_SHEET} gia' allineata")
            return 0
        if args.check:
            print(f"{REFS_SHEET} non e' allineata: {len(rows)} righe attese")
            return 1
        print(f"scritta {REFS_SHEET}: {len(rows)} relazioni")
        return 0

    if args.command == "deploy":
        if not args.wiki_root:
            print("deploy richiede --wiki-root <path del clone della Wiki>")
            return 1
        if not os.path.isdir(args.wiki_root):
            print(f"--wiki-root non e' una directory: {args.wiki_root}")
            return 1
        changed, missing = apply_wiki_deploy(data, args.wiki_root, check=not args.write)
        for note in missing:
            print(f"WARN  {note}")
        verb = "aggiornata" if args.write else "da aggiornare"
        for name in changed:
            print(f"{verb}: {name}")
        print(f"\npagine {verb}: {len(changed)}")
        if not args.write and changed:
            print("sola lettura: aggiungi --write per scrivere nel clone della Wiki")
        return 0

    if args.command == "report":
        print(render_audit(data))
        return 0

    return 0


if __name__ == "__main__":
    sys.exit(main())
