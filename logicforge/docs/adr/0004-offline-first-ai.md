# ADR-0004: Offline-first; AI tutor is a later, optional layer

- Status: Accepted
- Date: 2026-11-06

## Context

It is tempting to generate lessons or explanations with an LLM at runtime.

## Decision

v1 is fully offline. All lessons, quizzes and code are authored and verified (ADR-0003).
The "How AI works" track builds tiny models **from scratch in C++** (perceptron, MLP with backprop, bigram
text model), so learners understand the mechanism rather than an API.

A later milestone may add an optional "Ask the tutor" feature using a bring-your-own API key, with these rules:
- The key is never shipped in the APK. It is entered by the user and stored with Android Keystore encryption.
- The LLM's output is constrained to the app's diagram DSL or clearly labelled as "AI-generated, may be wrong".

## Why

- Runtime-generated content can be confidently wrong, which is the worst failure for a beginner.
- It costs money per use, and lessons would stop working without internet.
