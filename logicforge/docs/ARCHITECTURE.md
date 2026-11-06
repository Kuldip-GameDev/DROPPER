# Architecture

## Layers

```
app/ (Kotlin, Compose)          UI only: navigation, text, drawing scenes
   │ NativeBridge.call(method, json)
app/src/main/cpp/jni_bridge.cpp  ~40 lines: jstring <-> std::string, calls forge::Api
   │
core/  (C++20, no platform deps)
   bridge/   Api: method name -> handler lambda
   viz/      Scene (what to draw) + Recorder (collects Frames)
   algo/     instrumented algorithms + registry
   memlab/   pointer & lambda memory scenarios
   slot/     slot maths engine
   patterns/ compiled design-pattern examples
   ai/       tiny neural nets
   dsl/      English -> diagram parser
   learn/    quiz grading, spaced repetition, XP, streaks, skill tree
```

## The trace model (how every visual works)

An algorithm receives a `Recorder&`. After each meaningful step it calls e.g.
`rec.frame(scene, "Compare 5 and 1", "compare")`. A **Frame** is a *complete snapshot*:

```json
{ "caption": "Compare a[0]=5 and a[1]=1", "line": "compare",
  "scene": { "kind": "array", "cells": [5,1,4], "marks": {"0":"compare","1":"compare"},
             "pointers": {"i":0, "j":1} } }
```

Snapshots instead of diffs (Memento pattern):
- stepping backward is just `index - 1`, with no undo logic;
- every frame can be rendered in isolation, which makes it easy to test;
- the cost is memory, bounded by capping traces at `Recorder::kMaxFrames` (inputs are small in a teaching app).

## Scene kinds (and the matching Compose renderer)

| kind | used by |
|---|---|
| `array` | sorting, searching, two pointers, sliding window, stacks/queues, hashing buckets |
| `graph` | linked lists, trees, heaps, graphs, class diagrams (core computes x/y layout) |
| `grid` | slot reels, flood fill, DP tables, N-Queens |
| `memory` | Pointer & Lambda Lab: stack frames, heap blocks, arrows |
| `plot` | RNG histograms, RTP convergence, loss curves |

## Threading

The UI calls the bridge from `Dispatchers.Default`. Core handlers are pure functions of their input, except
`learn/` progress, which is guarded by a mutex and persisted as JSON under the app's `filesDir`.
