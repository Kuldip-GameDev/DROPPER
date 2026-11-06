# LogicForge

**Learn data structures, algorithms, design patterns and modern C++ by watching them run, then building them yourself.**
It's an Android app with a C++20 brain, written for game developers (slot-machine developers especially) and for complete beginners.

> Status: early development (v0.x). See [`CHANGELOG.md`](CHANGELOG.md) and [`docs/ROADMAP.md`](docs/ROADMAP.md).

---

## Why this exists

Most DSA apps show you a finished answer. LogicForge is built on three ideas:

1. **See it.** Every algorithm runs inside a C++ engine that records each step as a picture. You scrub
   forward and backward through those steps like a video, and the line of code that caused each one lights up.
2. **Understand it like you're 15.** Every lesson starts with an everyday analogy, then a picture, then
   the smallest possible piece of code. Jargon is only used after it has been explained.
3. **Build it.** Watching isn't learning. Every lesson ends with a *koan*: a small C++ function in
   [`exercises/`](exercises/) that you implement on your PC until its tests pass.

## What's inside (planned for v1)

| Track | What you learn | Why it matters for game dev |
|---|---|---|
| Algorithm Visualizer | sorting, searching, two pointers, sliding window, stacks, queues, linked lists, trees, heaps, hashing, graphs, DP, backtracking | Pathfinding, spatial queries, scheduling, AI |
| Pointer & Lambda Lab | stack vs heap, `T*`, `T**`, references, `const` placement, `new`/`delete`, leaks, dangling pointers, smart pointers, lambda captures | The #1 source of crashes in C++ games |
| Slot Math Lab | RNGs, weighted picks (linear, binary search, alias method), reel strips, paylines, 243 ways, cluster pays (flood fill), cascades, RTP (exact and Monte Carlo), volatility | The real maths behind the games you ship |
| Design Patterns | Command, Observer, State, Strategy, Factory, Flyweight, Object Pool, Game Loop, Component, Event Queue… | How large game codebases stay sane |
| Hard-to-read C++ | Decoding declarations, templates, bit tricks, dense STL chains | Reading engine and library code |
| English → Diagram | Type `tree 8 3 10 1 6` and see a binary search tree drawn | Turn the picture in your head into a structure |
| How AI works | A neural network written from scratch in C++, backprop one step at a time, a tiny next-word model | Knowing how AI works, not just how to call it |

Motivation: XP, levels, daily streaks, achievements, a skill tree, spaced-repetition reviews and *Topic Reels*,
a reel spin that picks today's review topic.

> The Slot Math Lab uses play credits only. It teaches the maths and engineering of slot games; it is not gambling.

## Architecture in one picture

```
 ┌──────────────────────────── Android app (Kotlin + Jetpack Compose) ───────────────────────────┐
 │  Screens: Home · Skill tree · Lesson · Visualizer · Labs · Profile                            │
 │  5 renderers draw whatever the core describes: Array · Graph · Grid · Memory · Plot           │
 └───────────────────────────────▲───────────────────────────────────────────────────────────────┘
                                 │  NativeBridge.call(method, argsJson) -> json   (one JNI function)
 ┌───────────────────────────────┴──────────── core/ (portable C++20) ───────────────────────────┐
 │  bridge/  dispatcher: map<string, function<json(const json&)>>                                 │
 │  viz/     Scene + TraceRecorder (every step is a snapshot -> free "step back")                 │
 │  algo/  memlab/  slot/  patterns/  ai/  dsl/  learn/                                           │
 └────────────────────────────────────────────────────────────────────────────────────────────────┘
```

The core has no Android dependencies. It builds and is tested on any desktop, which is also how you study it.
The reasoning behind each decision is recorded in [`docs/adr/`](docs/adr/).

## Build

### Core (any Linux/macOS/Windows machine with a C++20 compiler and CMake ≥ 3.20)

```bash
cmake -S core -B build -DFORGE_SANITIZE=ON     # ASan + UBSan catch pointer bugs for you
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/forge_cli list                          # every algorithm / scenario the core knows
./build/forge_cli trace sort.bubble 5 1 4 2 8   # watch bubble sort in your terminal
```

### Android app

Requires JDK 17+, Android SDK (platform 35) and NDK `27.3.13750724`.

```bash
./gradlew assembleDebug
adb install app/build/outputs/apk/debug/app-debug.apk
```

CI builds the APK on every push that touches `logicforge/` and attaches it to the
`logicforge-latest` pre-release.

### Exercises (koans)

```bash
cmake -S core -B build -DFORGE_EXERCISES=ON && cmake --build build -j
ctest --test-dir build -R ex_ --output-on-failure   # red until you implement them
```

## Moving LogicForge into its own repository

LogicForge currently lives in the `logicforge/` folder of the DROPPER repo. To give it its own repo **with full history**:

```bash
# inside DROPPER
git subtree split --prefix=logicforge -b logicforge-main
git push git@github.com:<you>/LogicForge.git logicforge-main:main
```

Then move `.github/workflows/logicforge.yml` into the new repo and delete its `working-directory: logicforge` lines.

## Project rules

- **No code is shown that hasn't been compiled and tested.** Snippets in the app are extracted from
  `core/` sources by [`scripts/extract_snippets.py`](scripts/extract_snippets.py). See ADR-0003.
- **Problems from LeetCode/HackerRank are linked, never copied.** Practice problems inside the app are original.
- Conventional Commits. See [`CONTRIBUTING.md`](CONTRIBUTING.md).

## License

MIT. See [`LICENSE`](LICENSE).
