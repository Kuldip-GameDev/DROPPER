# Exercises (koans)

Watching an algorithm is not the same as being able to write it. Each file here has:

1. a **TODO** function with a hint, and
2. **tests** underneath that fail until your implementation is right.

```bash
cmake -S core -B build -DFORGE_EXERCISES=ON -DFORGE_SANITIZE=ON
cmake --build build -j
ctest --test-dir build -R ex_ --output-on-failure      # all exercises
ctest --test-dir build -R ex_01 --output-on-failure    # just one
```

`-DFORGE_SANITIZE=ON` turns on AddressSanitizer. If you write past the end of an array or use a
dead pointer, you get a clear report instead of a random crash. Keep it on while learning.

Rules for yourself:
- Do not look at `core/src` until your tests pass. Then compare and see what you'd do differently.
- Write the loop invariant as a comment before the loop ("everything left of `i` is ...").
- Stuck for 20 minutes? Re-watch the matching visual in the app, step by step.

| # | Exercise | Skill |
|---|---|---|
| 01 | `swapValues(int*, int*)` | pointers, `*` and `&` |
| 02 | `reverseInPlace` | two pointers |
| 03 | `lowerBound` | binary search invariants |
| 04 | `countWindowsAtLeast` | sliding window |
| 05 | `reverseList` | linked lists, pointer rewiring |
