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
