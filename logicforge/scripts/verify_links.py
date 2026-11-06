#!/usr/bin/env python3
"""Check that every LeetCode / HackerRank link in content/ points at a real problem.

The problem pages themselves sit behind a bot challenge, so this uses the sites'
public JSON endpoints instead:
  * LeetCode:   POST https://leetcode.com/graphql  (question by titleSlug)
  * HackerRank: GET  https://www.hackerrank.com/rest/contests/master/challenges/<slug>

It also checks that the title written in the lesson matches the real title.
Not run in CI (external sites are flaky); run it before committing new links.
"""
from __future__ import annotations

import json
import re
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LEETCODE = re.compile(r"^https://leetcode\.com/problems/([a-z0-9-]+)/$")
HACKERRANK = re.compile(r"^https://www\.hackerrank\.com/challenges/([a-z0-9-]+)/problem$")
UA = "LogicForge-link-checker"


def leetcode_title(slug: str) -> str | None:
    body = json.dumps({
        "query": "query q($s: String!) { question(titleSlug: $s) { title } }",
        "variables": {"s": slug},
    }).encode()
    req = urllib.request.Request("https://leetcode.com/graphql", data=body, headers={
        "Content-Type": "application/json", "Referer": "https://leetcode.com", "User-Agent": UA})
    with urllib.request.urlopen(req, timeout=20) as r:
        q = json.load(r).get("data", {}).get("question")
    return q["title"] if q else None


def hackerrank_title(slug: str) -> str | None:
    req = urllib.request.Request(f"https://www.hackerrank.com/rest/contests/master/challenges/{slug}",
                                 headers={"User-Agent": UA})
    try:
        with urllib.request.urlopen(req, timeout=20) as r:
            return json.load(r).get("model", {}).get("name")
    except urllib.error.HTTPError:
        return None


def links():
    for path in sorted((ROOT / "content").rglob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        for card in data.get("cards", []):
            for link in card.get("links", []):
                yield path.relative_to(ROOT).as_posix(), link


def main() -> int:
    seen: dict[str, str | None] = {}
    bad = 0
    for where, link in links():
        url, title = link["url"], link["title"]
        if url not in seen:
            if m := LEETCODE.match(url):
                seen[url] = leetcode_title(m.group(1))
            elif m := HACKERRANK.match(url):
                seen[url] = hackerrank_title(m.group(1))
            else:
                seen[url] = None
        real = seen[url]
        if real is None:
            print(f"MISSING  {where}: {url}")
            bad += 1
        elif real != title:
            print(f"TITLE    {where}: lesson says '{title}', site says '{real}'")
            bad += 1
    print(f"checked {len(seen)} unique links, {bad} problem(s)")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
