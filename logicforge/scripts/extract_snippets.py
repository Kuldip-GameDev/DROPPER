#!/usr/bin/env python3
"""Extract teaching snippets from compiled C++ sources (see docs/adr/0003).

Markers inside core/ sources:

    // [snippet:sort.bubble]      start of a snippet
    ...code...  // @line compare  a line the visualizer can highlight
    rec.frame(...);  // @viz       recorder-only line: hidden from learners
    // [/snippet]                 end of a snippet

Usage:
    extract_snippets.py                 write app/src/main/assets/generated/snippets.json
    extract_snippets.py --check         fail if that file is stale or anything is inconsistent
    extract_snippets.py --check --traces build/traces.json
                                        also verify every trace frame's line label exists
                                        (traces.json comes from `forge_cli dump-traces`)
"""
from __future__ import annotations

import argparse
import json
import re
import sys
import textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCES = [ROOT / "core" / "src", ROOT / "core" / "include"]
OUT = ROOT / "app" / "src" / "main" / "assets" / "generated" / "snippets.json"
CONTENT = ROOT / "content"

START = re.compile(r"^\s*//\s*\[snippet:([A-Za-z0-9_.\-]+)\]\s*$")
END = re.compile(r"^\s*//\s*\[/snippet\]\s*$")
VIZ = re.compile(r"//\s*@viz\s*$")
FORMAT_TOGGLE = re.compile(r"^\s*//\s*clang-format\s+(on|off)\s*$")
LINE = re.compile(r"\s*//\s*@line\s+([A-Za-z0-9_]+)\s*$")
MAX_WIDTH = 100


class SnippetError(Exception):
    pass


def extract_file(path: Path) -> dict[str, dict]:
    snippets: dict[str, dict] = {}
    current_id: str | None = None
    start_line = 0
    body: list[str] = []
    labels: dict[str, int] = {}
    rel = path.relative_to(ROOT).as_posix()

    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        if (m := START.match(raw)) is not None:
            if current_id is not None:
                raise SnippetError(f"{rel}:{number}: snippet '{m.group(1)}' starts inside '{current_id}'")
            current_id, start_line, body, labels = m.group(1), number, [], {}
            continue
        if END.match(raw):
            if current_id is None:
                raise SnippetError(f"{rel}:{number}: [/snippet] without a start")
            # Drop blank lines at both ends and shift the label numbers to match.
            lead = 0
            while lead < len(body) and not body[lead].strip():
                lead += 1
            trail = len(body)
            while trail > lead and not body[trail - 1].strip():
                trail -= 1
            code = textwrap.dedent("\n".join(body[lead:trail]))
            shifted = {k: v - lead for k, v in labels.items()}
            snippets[current_id] = {"code": code, "lines": shifted, "file": rel, "sourceLine": start_line}
            current_id = None
            continue
        if current_id is None:
            continue
        if VIZ.search(raw) or FORMAT_TOGGLE.match(raw):
            continue  # recorder-only line / formatter switch: hidden from the learner
        line = raw.rstrip()
        if (m := LINE.search(line)) is not None:
            label = m.group(1)
            if label in labels:
                raise SnippetError(f"{rel}:{number}: label '{label}' used twice in '{current_id}'")
            line = line[: m.start()].rstrip()
            labels[label] = len(body) + 1  # 1-based line number in the displayed code
        body.append(line)

    if current_id is not None:
        raise SnippetError(f"{rel}: snippet '{current_id}' is never closed")

    return snippets


def extract_all() -> dict[str, dict]:
    result: dict[str, dict] = {}
    for base in SOURCES:
        for path in sorted(base.rglob("*")):
            if path.suffix not in {".cpp", ".hpp", ".h"}:
                continue
            for sid, snip in extract_file(path).items():
                if sid in result:
                    raise SnippetError(f"duplicate snippet id '{sid}' in {snip['file']} and {result[sid]['file']}")
                result[sid] = snip
    return dict(sorted(result.items()))


def lesson_snippet_refs() -> list[tuple[str, str]]:
    """Every "snippet": "<id>" value inside content/**/*.json."""
    refs: list[tuple[str, str]] = []

    def walk(node, where: str):
        if isinstance(node, dict):
            for key, value in node.items():
                if key == "snippet" and isinstance(value, str):
                    refs.append((value, where))
                else:
                    walk(value, where)
        elif isinstance(node, list):
            for item in node:
                walk(item, where)

    if CONTENT.exists():
        for path in sorted(CONTENT.rglob("*.json")):
            walk(json.loads(path.read_text(encoding="utf-8")), path.relative_to(ROOT).as_posix())
    return refs


CARD_TYPES = {"text", "visual", "code", "quiz", "build", "practice"}
LINK_HOSTS = ("https://leetcode.com/problems/", "https://www.hackerrank.com/challenges/")


def check_lessons(algos: dict[str, dict] | None) -> list[str]:
    """Structural checks on content/: the app must never crash on a lesson."""
    problems: list[str] = []
    lessons_dir = CONTENT / "lessons"
    lesson_ids: set[str] = set()
    for path in sorted(lessons_dir.glob("*.json")) if lessons_dir.exists() else []:
        where = path.relative_to(ROOT).as_posix()
        lesson = json.loads(path.read_text(encoding="utf-8"))
        if lesson.get("id") != path.stem:
            problems.append(f"{where}: id '{lesson.get('id')}' must match the file name '{path.stem}'")
        lesson_ids.add(path.stem)
        for key in ("title", "cards"):
            if not lesson.get(key):
                problems.append(f"{where}: missing '{key}'")
        for i, card in enumerate(lesson.get("cards", []), start=1):
            kind = card.get("type")
            at = f"{where} card {i}"
            if kind not in CARD_TYPES:
                problems.append(f"{at}: unknown card type '{kind}'")
            if kind == "quiz":
                choices = card.get("choices", [])
                if len(choices) < 2 or not 0 <= card.get("answer", -1) < len(choices):
                    problems.append(f"{at}: quiz needs >= 2 choices and a valid 'answer' index")
                if not card.get("explain"):
                    problems.append(f"{at}: quiz needs an 'explain' (learners learn from the why)")
            if kind == "visual" and algos is not None and card.get("algo") not in algos:
                problems.append(f"{at}: unknown algorithm '{card.get('algo')}'")
            if kind == "build" and not (ROOT / "exercises" / f"{card.get('exercise')}.cpp").exists():
                problems.append(f"{at}: exercise '{card.get('exercise')}' does not exist in exercises/")
            if kind == "practice":
                for link in card.get("links", []):
                    if not str(link.get("url", "")).startswith(LINK_HOSTS):
                        problems.append(f"{at}: link '{link.get('url')}' is not a LeetCode/HackerRank problem URL")
    tracks_path = CONTENT / "tracks.json"
    if tracks_path.exists():
        for track in json.loads(tracks_path.read_text(encoding="utf-8")).get("tracks", []):
            for lesson_id in track.get("lessons", []):
                if lesson_id not in lesson_ids:
                    problems.append(f"content/tracks.json: track '{track.get('id')}' lists missing lesson '{lesson_id}'")
    return problems


def check(snippets: dict[str, dict], traces_path: Path | None) -> list[str]:
    problems: list[str] = []

    for sid, snip in snippets.items():
        lines = snip["code"].split("\n")
        for label, n in snip["lines"].items():
            if not 1 <= n <= len(lines):
                problems.append(f"{sid}: label '{label}' points outside the snippet")
        for i, line in enumerate(lines, start=1):
            if len(line) > MAX_WIDTH:
                problems.append(f"{sid}:{i}: line is {len(line)} chars (max {MAX_WIDTH}, phones are narrow)")

    for sid, where in lesson_snippet_refs():
        if sid not in snippets:
            problems.append(f"{where}: unknown snippet '{sid}'")

    algos: dict[str, dict] | None = None
    if traces_path is not None:
        algos = {item["id"]: item for item in json.loads((traces_path.parent / "algos.json").read_text(encoding="utf-8"))}
    problems += check_lessons(algos)

    if traces_path is not None and algos is not None:
        traces = json.loads(traces_path.read_text(encoding="utf-8"))
        for algo_id, trace in traces.items():
            sid = algos.get(algo_id, {}).get("snippet", "")
            if sid not in snippets:
                problems.append(f"algorithm '{algo_id}' references unknown snippet '{sid}'")
                continue
            known = snippets[sid]["lines"]
            for step, frame in enumerate(trace["frames"], start=1):
                label = frame.get("line", "")
                if label and label not in known:
                    problems.append(f"{algo_id} step {step}: line label '{label}' not found in snippet '{sid}'")

    if OUT.exists():
        on_disk = json.loads(OUT.read_text(encoding="utf-8"))
        if on_disk != snippets:
            problems.append(f"{OUT.relative_to(ROOT)} is stale: run scripts/extract_snippets.py and commit it")
    else:
        problems.append(f"{OUT.relative_to(ROOT)} is missing: run scripts/extract_snippets.py")
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="verify instead of writing")
    parser.add_argument("--traces", type=Path, help="traces.json from `forge_cli dump-traces` (algos.json beside it)")
    args = parser.parse_args()

    try:
        snippets = extract_all()
    except SnippetError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    if args.check:
        problems = check(snippets, args.traces)
        for p in problems:
            print(f"error: {p}", file=sys.stderr)
        if not problems:
            print(f"ok: {len(snippets)} snippets, {len(lesson_snippet_refs())} lesson references")
        return 1 if problems else 0

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(snippets, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"wrote {len(snippets)} snippets to {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
