# Contributing to LogicForge

## Commit messages: Conventional Commits

```
<type>(<scope>): <summary in imperative mood, max ~72 chars>

<body: what and WHY, wrapped at 72>
```

| type | use for |
|---|---|
| `feat` | a new user-visible capability |
| `fix` | a bug fix |
| `docs` | documentation only |
| `test` | adding or fixing tests |
| `refactor` | code change that neither fixes a bug nor adds a feature |
| `build` / `ci` | build system / CI pipeline |
| `content` | lessons, quizzes, problem links |

Scopes: `core`, `algo`, `memlab`, `slot`, `patterns`, `ai`, `dsl`, `learn`, `bridge`, `app`, `content`, `exercises`.

One logical change per commit. A commit must build and pass tests on its own.

## Adding an algorithm (the whole flow)

1. Write it in `core/src/algo/<topic>.cpp` with `// [snippet:<id>]` markers around the teaching code.
   Recorder calls go on their own lines ending in `// @viz`, and lines the trace can point at get `// @line <label>`.
2. Register it in the same file (`FORGE_REGISTER_ALGO(...)`).
3. Add a test in `core/tests/` that checks the result against a reference (`std::sort`, brute force…),
   plus trace invariants such as "the last frame is sorted".
4. Run `python3 scripts/extract_snippets.py --check`.
5. Add or extend a lesson in `content/` that references the algorithm id and snippet id.

## Writing lessons

Read [`docs/CONTENT_STYLE.md`](docs/CONTENT_STYLE.md) first. The short version: one idea per card, an analogy before
the jargon, a picture before the code, and code before the formula.

## Code style

- C++20, `clang-format` (config in repo root). No raw `new`/`delete` outside the Pointer Lab teaching code.
- Kotlin: official style, 120 columns.
