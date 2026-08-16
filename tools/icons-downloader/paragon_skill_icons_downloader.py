#!/usr/bin/env python3
"""
Paragon Skill Icons Downloader

Sources:
1) Paragon Archive / Fandom Category:Abilities (archived media set)
2) Historical dump of Epic's Paragon developer API, which preserves original
   developer-paragon-cdn.epicgames.com icon URLs.

No third-party packages required.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from typing import Iterable
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode, urlparse
from urllib.request import Request, urlopen


USER_AGENT = "ParagonSkillIconArchiver/1.0 (+personal archival/reference use)"
FANDOM_API = "https://paragon-archive.fandom.com/api.php"
EPIC_DUMP = (
    "https://raw.githubusercontent.com/alex-taxiera/ParagoneAPI/"
    "master/src/api/paragone/data/heroes.json"
)


def http_get(url: str, timeout: int = 30) -> bytes:
    req = Request(url, headers={"User-Agent": USER_AGENT})
    with urlopen(req, timeout=timeout) as r:
        return r.read()


def api_json(params: dict) -> dict:
    url = FANDOM_API + "?" + urlencode(params)
    return json.loads(http_get(url).decode("utf-8"))


def safe_name(value: str) -> str:
    value = value.strip()
    value = re.sub(r'[<>:"/\\|?*\x00-\x1f]', "_", value)
    value = re.sub(r"\s+", " ", value).strip(" .")
    return value or "unnamed"


def download_with_retries(url: str, dest: Path, retries: int = 3) -> tuple[bool, str]:
    dest.parent.mkdir(parents=True, exist_ok=True)
    last_error = ""
    for attempt in range(1, retries + 1):
        try:
            data = http_get(url)
            dest.write_bytes(data)
            return True, ""
        except (HTTPError, URLError, TimeoutError, OSError) as exc:
            last_error = f"{type(exc).__name__}: {exc}"
            if attempt < retries:
                time.sleep(1.5 * attempt)
    return False, last_error


def collect_fandom_files() -> list[str]:
    """Return every File:* title in Category:Abilities."""
    titles: list[str] = []
    cmcontinue = None

    while True:
        params = {
            "action": "query",
            "format": "json",
            "list": "categorymembers",
            "cmtitle": "Category:Abilities",
            "cmtype": "file",
            "cmlimit": "500",
        }
        if cmcontinue:
            params["cmcontinue"] = cmcontinue

        payload = api_json(params)
        titles.extend(item["title"] for item in payload["query"]["categorymembers"])

        cont = payload.get("continue")
        if not cont:
            break
        cmcontinue = cont["cmcontinue"]

    return titles


def chunks(values: list[str], size: int) -> Iterable[list[str]]:
    for i in range(0, len(values), size):
        yield values[i:i + size]


def resolve_fandom_urls(titles: list[str]) -> list[dict]:
    """
    Resolve original media URLs through MediaWiki imageinfo.
    Batch size 50 works for anonymous API calls.
    """
    items: list[dict] = []
    for batch in chunks(titles, 50):
        payload = api_json(
            {
                "action": "query",
                "format": "json",
                "prop": "imageinfo",
                "iiprop": "url|size|mime",
                "titles": "|".join(batch),
            }
        )
        for page in payload["query"]["pages"].values():
            title = page.get("title", "")
            info = (page.get("imageinfo") or [{}])[0]
            url = info.get("url")
            if not url:
                continue
            items.append(
                {
                    "source": "paragon-archive",
                    "hero": "",
                    "ability": title.removeprefix("File:"),
                    "binding": "",
                    "type": "",
                    "url": url,
                    "width": info.get("width", ""),
                    "height": info.get("height", ""),
                    "mime": info.get("mime", ""),
                }
            )
    items.sort(key=lambda x: x["ability"].casefold())
    return items


def collect_epic_api_icons() -> list[dict]:
    """
    Read the historical Paragon API dump and collect ability icons that point
    to Epic's original developer CDN.
    """
    heroes = json.loads(http_get(EPIC_DUMP).decode("utf-8"))
    items: list[dict] = []

    for hero in heroes:
        hero_name = hero.get("name", "Unknown")
        for index, ability in enumerate(hero.get("abilities") or [], start=1):
            icon = ((ability.get("images") or {}).get("icon") or "").strip()
            if not icon:
                continue
            if icon.startswith("//"):
                icon = "https:" + icon
            items.append(
                {
                    "source": "epic-api-dump",
                    "hero": hero_name,
                    "ability": ability.get("name", f"Ability {index}"),
                    "binding": ability.get("binding", ""),
                    "type": ability.get("type", ""),
                    "url": icon,
                    "width": "",
                    "height": "",
                    "mime": "",
                    "_index": index,
                }
            )

    items.sort(key=lambda x: (x["hero"].casefold(), x["_index"]))
    return items


def choose_dest(root: Path, item: dict) -> Path:
    parsed = urlparse(item["url"])
    ext = Path(parsed.path).suffix.lower()
    if not ext or len(ext) > 8:
        ext = ".png"

    if item["source"] == "epic-api-dump":
        hero = safe_name(item["hero"])
        binding = safe_name(item["binding"]) if item["binding"] else "slot"
        idx = int(item.get("_index", 0))
        filename = f"{idx:02d}_{binding}_{safe_name(item['ability'])}{ext}"
        return root / "Epic_CDN" / hero / filename

    filename = safe_name(item["ability"])
    # File title already carries an extension most of the time.
    if not Path(filename).suffix:
        filename += ext
    return root / "Paragon_Archive" / filename


def write_manifest(root: Path, rows: list[dict]) -> Path:
    manifest = root / "manifest.csv"
    columns = [
        "source", "hero", "ability", "binding", "type",
        "url", "local_path", "status", "error",
        "width", "height", "mime",
    ]
    with manifest.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=columns)
        writer.writeheader()
        for row in rows:
            writer.writerow({k: row.get(k, "") for k in columns})
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Download archived Paragon hero skill/ability icons."
    )
    parser.add_argument(
        "--source",
        choices=["archive", "epic", "both"],
        default="both",
        help="archive=Fandom 105-file category, epic=historical Epic CDN links, both=both",
    )
    parser.add_argument(
        "--out",
        default="Paragon_Skill_Icons",
        help="Output directory (default: Paragon_Skill_Icons)",
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=8,
        help="Concurrent downloads (default: 8)",
    )
    parser.add_argument(
        "--list-only",
        action="store_true",
        help="Create the manifest without downloading image files",
    )
    args = parser.parse_args()

    root = Path(args.out).resolve()
    root.mkdir(parents=True, exist_ok=True)

    items: list[dict] = []

    if args.source in ("archive", "both"):
        print("Reading Paragon Archive Category:Abilities...")
        titles = collect_fandom_files()
        print(f"  Found {len(titles)} archived media files.")
        items.extend(resolve_fandom_urls(titles))

    if args.source in ("epic", "both"):
        print("Reading historical Epic Paragon API dump...")
        epic_items = collect_epic_api_icons()
        print(f"  Found {len(epic_items)} ability icon references.")
        items.extend(epic_items)

    rows: list[dict] = []
    tasks: list[tuple[dict, Path]] = []

    for item in items:
        dest = choose_dest(root, item)
        row = dict(item)
        row["local_path"] = str(dest.relative_to(root))
        row["status"] = "listed" if args.list_only else "pending"
        row["error"] = ""
        rows.append(row)
        if not args.list_only:
            tasks.append((row, dest))

    if not args.list_only:
        print(f"Downloading {len(tasks)} files with {args.workers} workers...")
        with ThreadPoolExecutor(max_workers=max(1, args.workers)) as pool:
            future_map = {
                pool.submit(download_with_retries, row["url"], dest): (row, dest)
                for row, dest in tasks
            }
            done = 0
            for future in as_completed(future_map):
                row, dest = future_map[future]
                ok, error = future.result()
                row["status"] = "ok" if ok else "failed"
                row["error"] = error
                done += 1
                if done % 20 == 0 or done == len(tasks):
                    print(f"  {done}/{len(tasks)}")

    manifest = write_manifest(root, rows)

    ok_count = sum(1 for r in rows if r["status"] == "ok")
    failed_count = sum(1 for r in rows if r["status"] == "failed")
    print()
    print(f"Manifest: {manifest}")
    if args.list_only:
        print(f"Listed: {len(rows)}")
    else:
        print(f"Downloaded: {ok_count}")
        print(f"Failed:     {failed_count}")
        if failed_count:
            print("Open manifest.csv to see failed URLs and error messages.")

    return 0 if failed_count == 0 else 2


if __name__ == "__main__":
    raise SystemExit(main())
