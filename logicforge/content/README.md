# Lesson content

Lessons are JSON, validated in CI by `scripts/extract_snippets.py --check`.
Write them using [`docs/CONTENT_STYLE.md`](../docs/CONTENT_STYLE.md).

```
content/
  tracks.json            ordered lesson ids per track
  lessons/<id>.json      one lesson; file name == "id"
```

## Lesson

```json
{ "id": "dsa.bubble_sort", "title": "Bubble sort", "summary": "one line", "minutes": 7, "cards": [ ... ] }
```

## Card types

| type | fields | notes |
|---|---|---|
| `text` | `title?`, `body` | body supports `**bold**`, `` `code` ``, blank-line paragraphs, `- ` bullets |
| `visual` | `algo`, `input?`, `title?`, `body?` | runs the C++ algorithm and shows the trace player; `algo` must exist (`forge_cli list`) |
| `code` | `snippet`, `highlight?`, `title?`, `body?` | `snippet` id from compiled sources; `highlight` is a `// @line` label |
| `quiz` | `question`, `choices[]`, `answer` (0-based), `explain`, `code?` | always explain *why* |
| `build` | `exercise`, `body`, `title?` | `exercise` = file name in `exercises/` without `.cpp` |
| `practice` | `links[]` of `{site, title, url, level}`, `body?` | LeetCode/HackerRank problem URLs only; link, never copy |

Before adding a practice link, check that it exists (`python3 scripts/verify_links.py`).
