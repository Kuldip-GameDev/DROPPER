# ADR-0001: Portable C++ core + thin Jetpack Compose UI

- Status: Accepted
- Date: 2026-11-06

## Context

The app has to be "built in C++", run on Android, and teach C++ through its own source code. The content is
mostly text (lessons, quizzes) plus animated diagrams.

## Options considered

| Option | Verdict |
|---|---|
| **A. C++20 core + Kotlin/Compose UI via JNI** | Chosen |
| B. Pure C++ (NativeActivity + Dear ImGui or raylib) | Rejected |
| C. Pure Kotlin | Rejected |
| D. WebView + Emscripten | Rejected |

**A.** All logic (algorithms, traces, memory simulator, slot maths, neural net, parser, quiz/SRS/XP rules) lives in C++.
Compose only draws scenes the core describes and handles navigation and text.
It is testable on desktop with sanitizers, the C++ is the part worth studying, and the UI gets native text rendering,
accessibility and keyboard input for free. The cost is a second language (Kotlin) and a JNI boundary, which ADR-0002 keeps small.

**B.** Great for a game loop, poor for reading. Text layout, scrolling, IME and accessibility would all have to be built by hand,
which is about 3x the UI work for a worse reading experience. *Revisit* for a future "mini-game" screen that embeds
a native surface.

**C.** Fastest to build, but it defeats the purpose: the app's own code would teach nothing about C++.

**D.** Three toolchains (C++/wasm, JS, Android) for a worse feel.

## Consequences

- `core/` must not include any Android headers. CI builds and tests it on plain Ubuntu.
- The UI is rewritable later (e.g. Compose Multiplatform for desktop/iOS) without touching the core.
