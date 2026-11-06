# Changelog

All notable changes to LogicForge are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), versioning: [SemVer](https://semver.org/).

## [Unreleased]

### Added
- Project bootstrap: README, license, contributing guide, architecture decision records, CI workflow.
- C++20 core: scene model (array, graph, grid), trace recorder, algorithm registry, JSON API dispatcher.
- 14 instrumented algorithms: bubble, selection, insertion, merge, quick and heap sort; linear and binary
  search; two pointers, sliding window, prefix sums; reverse linked list, Floyd cycle detection; balanced brackets.
- `forge_cli` terminal tool (`list`, `trace`, `call`, `dump-traces`).
- Snippet extractor: code shown in the app is extracted from compiled sources; CI checks every trace's line labels.
- Five exercises (koans): pointer swap, two-pointer reverse, lower bound, sliding window, reverse list.
- Android app (Kotlin, Jetpack Compose): home, lesson player, Algorithm Lab, trace player with
  play/step/scrub/speed, syntax-highlighted code panel that follows the trace, renderers for array, bar,
  stack, graph and grid scenes. One JNI function carries UTF-8 JSON bytes to the C++ core.
- 12 beginner lessons (Foundations track) with visuals, compiled code, quizzes, build exercises and
  34 verified LeetCode/HackerRank practice links.
- Lesson validation in CI and `scripts/verify_links.py` to check practice links against the real sites.
- Pointer & Lambda Lab: 15 scenarios of real, running C++ drawn as stack frames, heap blocks and arrows
  (address-of, references, pass-by, pointer arithmetic, `T**`, new/delete, leaks, dangling pointers,
  unique/shared/weak_ptr, lambda captures, `mutable`, dangling captures, comparators, callbacks).
- 8-lesson Pointer & Lambda track and two new exercises (smart pointers, lambdas).
