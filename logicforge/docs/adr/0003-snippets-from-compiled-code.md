# ADR-0003: Every code snippet comes from compiled, tested code

- Status: Accepted
- Date: 2026-11-06

## Context

A learning app that shows wrong code teaches wrong things. Hand-pasting code into lesson text drifts out of date
and is never compiled.

## Decision

Code shown in the app is **extracted** from `core/` sources at build time by `scripts/extract_snippets.py`:

```cpp
// [snippet:sort.bubble]
void bubbleSort(std::vector<int>& a, Recorder& rec) {
    for (std::size_t i = 0; i + 1 < a.size(); ++i) {        // @line outer
        rec.step("pass", i);                                  // @viz
        ...
// [/snippet]
```

- Lines ending in `// @viz` are recorder calls. They are removed from the displayed code.
- `// @line <label>` marks a line the trace can point at. The extractor turns labels into line numbers *in the
  displayed code*, so the highlight is always on the right line.
- `--check` fails CI if a lesson references a snippet that doesn't exist or a label is used twice.

## Consequences

- The code a learner reads is the code the visualizer runs, and it is covered by tests.
- Teaching code must stay readable even though it is also production code. That is a feature, not a cost.
