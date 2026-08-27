#!/usr/bin/env python3
"""Genera una vista HTML unica del Decision Log (`D-nnn`).

Legge l'owner canonico `docs/decisions/RT_PDR_00_Decision_Log.md` e produce una
pagina autonoma (nessuna risorsa esterna) con ricerca, filtri di stato e link
cliccabili verso le issue GitHub, gli altri `D-nnn` e i documenti del repository.

    python3 tools/decision-log/build_decision_view.py --out build/decision-log.html

Il file HTML e' un artefatto generato: non va committato.
"""

from __future__ import annotations

import argparse
import html
import json
import posixpath
import re
from pathlib import Path

REPO = "DegrassiAaron/refactor-tactics-main"
BLOB = f"https://github.com/{REPO}/blob/main"
ISSUE = f"https://github.com/{REPO}/issues"

LOG_REL = "docs/decisions/RT_PDR_00_Decision_Log.md"
LOG_DIR = "docs/decisions"

ROW_RE = re.compile(r"^\|\s*\*{0,2}~{0,2}D-\d{3}")
ID_RE = re.compile(r"D-(\d{3})")
DATE_RE = re.compile(r"\d{4}-\d{2}-\d{2}")
ISSUE_RE = re.compile(r"#(\d{2,5})\b")
DREF_RE = re.compile(r"\bD-(\d{3})\b")


# --------------------------------------------------------------------------- md

class Inline:
    """Renderer inline minimale: code, link, bold, italic, strike, autolink."""

    def __init__(self) -> None:
        self.tokens: list[str] = []

    def _tok(self, htm: str) -> str:
        self.tokens.append(htm)
        return f"\x00{len(self.tokens) - 1}\x00"

    def _restore(self, text: str) -> str:
        return re.sub(r"\x00(\d+)\x00", lambda m: self.tokens[int(m.group(1))], text)

    # -- target di un link markdown -> URL utilizzabile dalla pagina
    @staticmethod
    def resolve(target: str, label_plain: str) -> tuple[str, bool]:
        """Ritorna (href, is_internal_decision)."""
        target = target.strip()
        if target.startswith(("http://", "https://", "mailto:")):
            return target, False
        anchor = ""
        if "#" in target:
            target, anchor = target.split("#", 1)
        if not target:  # link puramente ad ancora
            return "#" + anchor, False
        resolved = posixpath.normpath(posixpath.join(LOG_DIR, target))
        if resolved == LOG_REL:
            m = ID_RE.search(label_plain)
            if m:
                return f"#d-{m.group(1)}", True
            return ("#d-" + anchor) if anchor else "#top", False
        href = f"{BLOB}/{resolved}"
        if anchor:
            href += "#" + anchor
        return href, False

    def render(self, text: str, autolink: bool = True) -> str:
        # 1) code spans (protetti da ogni altra trasformazione)
        def code(m: re.Match[str]) -> str:
            body = m.group(2)
            esc = html.escape(body)
            im = ISSUE_RE.fullmatch(body.strip())
            if im:
                return self._tok(
                    f'<a class="ref ref-issue" href="{ISSUE}/{im.group(1)}" '
                    f'target="_blank" rel="noopener"><code>{esc}</code></a>'
                )
            dm = DREF_RE.fullmatch(body.strip())
            if dm:
                return self._tok(
                    f'<a class="ref ref-dec" href="#d-{dm.group(1)}" '
                    f'data-dec="{dm.group(1)}"><code>{esc}</code></a>'
                )
            return self._tok(f"<code>{esc}</code>")

        # ``code con `backtick` dentro`` prima dei code span semplici
        text = re.sub(r"(`{1,2})(.+?)\1", code, text)

        # 2) link markdown
        def link(m: re.Match[str]) -> str:
            label, target = m.group(1), m.group(2)
            plain = self._restore(label)
            plain = re.sub(r"<[^>]+>", "", plain)
            href, internal = self.resolve(target, plain)
            inner = self.render(label, autolink=False)
            cls = "ref ref-dec" if internal else "lnk"
            attrs = ""
            if internal:
                attrs = f' data-dec="{ID_RE.search(plain).group(1)}"'
            elif href.startswith("http"):
                attrs = ' target="_blank" rel="noopener"'
                if href.startswith(ISSUE.rsplit("/", 1)[0]):
                    cls = "ref ref-issue"
            return self._tok(f'<a class="{cls}" href="{href}"{attrs}>{inner}</a>')

        text = re.sub(r"\[([^\]]*)\]\(([^)]+)\)", link, text)

        # 3) escape del testo residuo
        text = html.escape(text, quote=False)

        # 4) enfasi
        # non-greedy: il grassetto puo' contenere corsivi annidati
        text = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", text)
        text = re.sub(r"~~(.+?)~~", r"<del>\1</del>", text)
        text = re.sub(r"(?<!\*)\*([^*\n]+)\*(?!\*)", r"<em>\1</em>", text)

        # 5) autolink su testo nudo
        if autolink:
            text = ISSUE_RE.sub(
                lambda m: f'<a class="ref ref-issue" href="{ISSUE}/{m.group(1)}" '
                          f'target="_blank" rel="noopener">#{m.group(1)}</a>',
                text,
            )
            text = DREF_RE.sub(
                lambda m: f'<a class="ref ref-dec" href="#d-{m.group(1)}" '
                          f'data-dec="{m.group(1)}">D-{m.group(1)}</a>',
                text,
            )

        return self._restore(text)


def md_inline(text: str) -> str:
    return Inline().render(text)


def md_block(lines: list[str]) -> str:
    """Rende un blocco (paragrafi + blockquote) in HTML."""
    out: list[str] = []
    buf: list[str] = []
    mode = "p"

    def flush() -> None:
        if not buf:
            return
        body = md_inline(" ".join(buf).strip())
        out.append(f"<blockquote>{body}</blockquote>" if mode == "q" else f"<p>{body}</p>")
        buf.clear()

    for raw in lines:
        line = raw.rstrip()
        if not line.strip():
            flush()
            continue
        is_q = line.lstrip().startswith(">")
        if is_q != (mode == "q"):
            flush()
            mode = "q" if is_q else "p"
        buf.append(line.lstrip().lstrip(">").strip() if is_q else line.strip())
    flush()
    return "".join(out)


COMMENT_RE = re.compile(r"<!--.*?-->", re.S)


def plain(text: str) -> str:
    """Testo semplice: usato per titolo compatto e indice di ricerca."""
    text = re.sub(r"`([^`]+)`", r"\1", text)
    text = re.sub(r"\[([^\]]*)\]\([^)]+\)", r"\1", text)
    text = re.sub(r"[*~]", "", text)
    return re.sub(r"\s+", " ", text).strip()


# ----------------------------------------------------------------------- parsing

def split_row(line: str) -> list[str]:
    """Splitta una riga di tabella ignorando le pipe dentro i code span."""
    cells: list[str] = []
    cur: list[str] = []
    tick = False
    for ch in line.strip().strip("|"):
        if ch == "`":
            tick = not tick
        if ch == "|" and not tick:
            cells.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    cells.append("".join(cur).strip())
    return cells


STATE_RULES = [
    ("superata", ("superata", "superato", "sostituita", "annullata", "ritirata")),
    ("aperta", ("open question", "aperta", "da bloccare", "non applicata")),
    ("proposta", ("proposta", "proposed", "playtest")),
    ("consolidata", ("consolidata", "consolidato")),
    ("accettata", ("accettata", "accettato", "eseguita", "implementata")),
]


def classify(state_md: str, struck: bool) -> str:
    s = plain(state_md).lower()
    if struck or s.startswith("superata") or "superata da" in s:
        return "superata"
    for key, needles in STATE_RULES:
        if any(n in s for n in needles):
            return key
    return "altra"


def split_lead(title: str) -> tuple[str, str]:
    """Separa la frase guida (il grassetto d'apertura) dal resto del testo."""
    m = re.match(r"\*\*(.+?)\*\*\s*", title, re.S)
    if m and len(plain(m.group(1))) <= 220:
        return plain(m.group(1)), title[m.end():].strip()
    flat = plain(title)
    cut = re.split(r"(?<=[.!?])\s+(?=[A-Z«`\u00C0-\u00DD])", flat, maxsplit=1)[0]
    if len(cut) > 190:
        cut = cut[:187].rsplit(" ", 1)[0] + "\u2026"
    return cut, title


def parse(md_path: Path) -> dict:
    text = md_path.read_text(encoding="utf-8")
    lines = text.split("\n")

    decisions: list[dict] = []
    for line in lines:
        if not ROW_RE.match(line):
            continue
        cells = split_row(COMMENT_RE.sub("", line))
        if len(cells) < 4:
            continue
        raw_id, title, state, impact = cells[0], cells[1], cells[2], " | ".join(cells[3:])
        num = ID_RE.search(raw_id).group(1)
        struck = "~~" in raw_id
        blob = f"{title} {state} {impact}"
        issues = sorted({int(n) for n in ISSUE_RE.findall(plain(blob))})
        refs = sorted({n for n in DREF_RE.findall(plain(blob)) if n != num})
        dates = sorted(set(DATE_RE.findall(f"{state} {title}")))
        cut, body_md = split_lead(title)
        decisions.append(
            {
                "id": f"D-{num}",
                "num": int(num),
                "slug": num,
                "headline": cut,
                "titleHtml": md_block([body_md]) if body_md else "",
                "stateHtml": md_inline(state),
                "stateText": plain(state),
                "bucket": classify(state, struck),
                "impactHtml": md_inline(impact),
                "impactText": plain(impact),
                "issues": issues,
                "refs": refs,
                "date": dates[-1] if dates else "",
                "struck": struck,
                "search": plain(blob).lower(),
                "notes": [],
            }
        )

    # --- sezione Note
    notes: list[dict] = []
    try:
        start = lines.index("## Note") + 1
    except ValueError:
        start = len(lines)
    item: list[str] | None = None
    for raw in lines[start:]:
        if raw.startswith("#"):
            break
        if raw.startswith("- "):
            if item:
                notes.append(build_note(item))
            item = [raw[2:]]
        elif item is not None:
            item.append(raw[2:] if raw.startswith("  ") else raw)
    if item:
        notes.append(build_note(item))

    by_id = {d["slug"]: d for d in decisions}
    orphans: list[dict] = []
    for note in notes:
        owners = [n for n in note["ids"] if n in by_id]
        if owners:
            by_id[owners[0]]["notes"].append(note["html"])
        else:
            orphans.append(note)

    # --- backlink: chi cita chi
    for d in decisions:
        d["citedBy"] = []
    for d in decisions:
        for r in d["refs"]:
            if r in by_id:
                by_id[r]["citedBy"].append(d["slug"])
    for d in decisions:
        d["citedBy"] = sorted(set(d["citedBy"]))
        d["refs"] = [r for r in d["refs"] if r in by_id]
        d["search"] += " " + " ".join(plain(n) for n in d["notes"]).lower()

    present = {d["num"] for d in decisions}
    missing = [n for n in range(1, max(present) + 1) if n not in present]

    return {
        "decisions": decisions,
        "orphanNotes": [n["html"] for n in orphans],
        "missing": missing,
        "source": LOG_REL,
        "sourceUrl": f"{BLOB}/{LOG_REL}",
        "repo": REPO,
    }


def build_note(item: list[str]) -> dict:
    ids = DREF_RE.findall(plain(item[0]))
    return {"ids": ids, "html": md_block(item)}


# ---------------------------------------------------------------------- template

TEMPLATE_PATH = Path(__file__).with_name("template.html")


def render_page(data: dict) -> str:
    tpl = TEMPLATE_PATH.read_text(encoding="utf-8")
    payload = json.dumps(data, ensure_ascii=False).replace("</", "<\\/")
    return tpl.replace("/*__DATA__*/null", payload)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--log", default=LOG_REL, type=Path, help="path del Decision Log")
    ap.add_argument("--out", default=Path("build/decision-log.html"), type=Path)
    args = ap.parse_args()

    data = parse(args.log)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(render_page(data), encoding="utf-8")

    buckets: dict[str, int] = {}
    for d in data["decisions"]:
        buckets[d["bucket"]] = buckets.get(d["bucket"], 0) + 1
    issues = {i for d in data["decisions"] for i in d["issues"]}
    print(f"{args.out}: {len(data['decisions'])} decisioni, {len(issues)} issue collegate")
    print("  stati: " + ", ".join(f"{k}={v}" for k, v in sorted(buckets.items())))
    if data["missing"]:
        print("  ID mancanti: " + ", ".join(f"D-{n:03d}" for n in data["missing"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
