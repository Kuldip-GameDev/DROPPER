# Content style guide: "explain it to a smart 15-year-old"

Every lesson is written for someone who has **never programmed before** and is also useful to someone who has.

## The card sequence

1. **Hook** – one sentence: why should I care? Preferably game-related.
2. **Analogy** – everyday picture first (a stack of plates, a row of lockers, a slot reel).
3. **Picture** – the live visual (trace) with 3–8 steps.
4. **Smallest code** – the snippet, max ~15 lines, with the active line highlighted.
5. **Check** – 1–3 quiz questions (predict the output, reorder the lines, spot the bug).
6. **Build** – a koan in `exercises/` with tests.
7. **Go further** – 2–4 classic problems on LeetCode / HackerRank (links only).

## Writing rules

- One idea per card. If you write "also", start a new card.
- Sentences under ~20 words. Active voice. "You", not "the programmer".
- Never use a term before it is explained. First use of a term is **bold** with a one-line meaning.
- Analogy before jargon, picture before code, code before formula.
- Big-O is introduced with counting ("how many times does this line run?") before notation.
- Show the common mistake and why it fails. Beginners learn as much from the wrong version.
- No "simply", "just", "obviously", "trivially".
- Gamedev examples where honest: inventories (arrays), undo (stacks), event queues, pathfinding (graphs), reels (circular arrays).

## Accuracy rules

- Code: only via snippet ids (ADR-0003). Never paste code into lesson text.
- Complexity claims must match the code shown (e.g. say "average O(n log n), worst O(n²)" for quicksort).
- External problems: link by URL slug, never copy statements.
