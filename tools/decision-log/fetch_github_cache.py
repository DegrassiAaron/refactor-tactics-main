#!/usr/bin/env python3
"""Aggiorna la cache GitHub dei riferimenti citati dal Decision Log.

Legge i `#nnnn` che compaiono in `docs/decisions/RT_PDR_00_Decision_Log.md` e per
ognuno chiede a GitHub stato, titolo e natura (issue o pull request), scrivendo
`tools/decision-log/github-cache.json`. La vista HTML legge quel file: senza, i
riferimenti restano link nudi.

    gh auth status                      # serve la CLI gh autenticata
    python3 tools/decision-log/fetch_github_cache.py

L'endpoint `repos/{owner}/{repo}/issues/{n}` risponde anche per le PR — il campo
`pull_request` è ciò che le distingue.
"""

from __future__ import annotations

import argparse
import datetime
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = "DegrassiAaron/refactor-tactics-main"
LOG = Path("docs/decisions/RT_PDR_00_Decision_Log.md")
OUT = Path("tools/decision-log/github-cache.json")
ISSUE_RE = re.compile(r"#(\d{2,5})\b")


def referenced(log: Path) -> list[int]:
    text = re.sub(r"<!--.*?-->", "", log.read_text(encoding="utf-8"), flags=re.S)
    rows = [l for l in text.split("\n") if re.match(r"^\|\s*\*{0,2}~{0,2}D-\d{3}", l)]
    notes = text.split("## Note")[-1] if "## Note" in text else ""
    return sorted({int(n) for n in ISSUE_RE.findall("\n".join(rows) + notes)})


def fetch(number: int, repo: str) -> dict | None:
    cmd = ["gh", "api", f"repos/{repo}/issues/{number}"]
    try:
        raw = subprocess.run(cmd, capture_output=True, text=True, check=True).stdout
    except FileNotFoundError:
        sys.exit("gh non è installato: serve la GitHub CLI autenticata")
    except subprocess.CalledProcessError as exc:
        print(f"  #{number}: {exc.stderr.strip().splitlines()[-1:] or 'errore'}", file=sys.stderr)
        return None
    data = json.loads(raw)
    pr = data.get("pull_request")
    return {
        "state": data["state"],
        "title": data["title"],
        "type": "pr" if pr else "issue",
        **({"merged": bool(pr.get("merged_at"))} if pr else {}),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--log", default=LOG, type=Path)
    ap.add_argument("--out", default=OUT, type=Path)
    ap.add_argument("--repo", default=REPO)
    args = ap.parse_args()

    numbers = referenced(args.log)
    print(f"{len(numbers)} riferimenti citati dal registro")

    items: dict[str, dict] = {}
    for i, n in enumerate(numbers, 1):
        entry = fetch(n, args.repo)
        if entry:
            items[str(n)] = entry
        if i % 25 == 0:
            print(f"  {i}/{len(numbers)}")

    args.out.write_text(
        json.dumps(
            {
                "repo": args.repo,
                "fetched": datetime.date.today().isoformat(),
                "items": items,
            },
            ensure_ascii=False,
            indent=1,
        )
        + "\n",
        encoding="utf-8",
    )
    opened = sum(1 for v in items.values() if v["state"] == "open")
    print(f"{args.out}: {len(items)} voci, {opened} aperte")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
