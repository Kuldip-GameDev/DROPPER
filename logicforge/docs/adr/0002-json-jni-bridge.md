# ADR-0002: One JNI function, JSON messages

- Status: Accepted
- Date: 2026-11-06

## Context

The UI needs dozens of operations (run a trace, list lessons, grade a quiz, run a slot simulation…).

## Decision

Expose exactly one native function:

```kotlin
external fun call(method: String, argsJson: String): String   // returns JSON
```

On the C++ side `forge::Api` holds a `std::unordered_map<std::string, std::function<Json(const Json&)>>`.
Each feature registers its handlers as lambdas. Errors come back as `{"error": "..."}`, never as a crash
across the JNI boundary (every handler runs inside `try/catch`).

## Why not one JNI function per operation?

- Every JNI function is hand-written boilerplate in two languages, with manual reference management.
- One function means one place to test. `forge_cli` and the core tests call the very same dispatcher.

## Cost

JSON encode/decode on every call. A trace of 2,000 frames is a few hundred KB, which parses in milliseconds on a phone.
Calls run on a background coroutine. If profiling ever shows a hot path, that method can get a dedicated binary endpoint.

## Dependency

[nlohmann/json](https://github.com/nlohmann/json) v3.11.3 (MIT), header-only, fetched by CMake `FetchContent`.
