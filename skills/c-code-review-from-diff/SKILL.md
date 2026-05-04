---
name: c-code-review-from-diff
description: Use when reviewing C/C++ embedded code changes from git diff. Triggered by requests to review changed C/C++ functions, check embedded-specific issues (interrupt safety, memory management, HAL boundaries), or produce a severity-annotated code review report.
---

# C/C++ Embedded Code Review from Git Diff

## Workflow

1. Get the requested diff.
2. Identify changed C/C++ functions from `@@` hunks.
3. Review only changed code unless surrounding context is required to prove an issue.
4. Apply embedded and general quality checks.
5. Output a severity-annotated Markdown report.

```bash
git diff HEAD          # unstaged changes
git diff HEAD~1 HEAD   # last commit
git diff main...HEAD   # branch changes
```

## Diff Parsing

From each `@@` hunk, identify:

- New or modified functions
- Changed function signatures
- New conditionals (`if`/`else`/`switch`) — note for branch coverage
- New external calls — potential HAL boundary violations

## Embedded Checks

Treat a function as an ISR only with strong evidence: `*_IRQHandler`, interrupt/isr attributes, vector-table entry, or registration via `NVIC_SetVector` or equivalent. A generic `*_Handler` name alone is ambiguous; emit `[INFO]` instead of applying ISR-only severities.

| Check | Pattern to detect | Severity |
| ----- | ----------------- | -------- |
| malloc/free in ISR | `malloc`, `free`, `new`, `delete` inside ISR handler | CRITICAL |
| Blocking call in ISR | `HAL_Delay`, `osDelay`, blocking I/O inside ISR body | CRITICAL |
| printf/scanf in ISR | `printf`, `sprintf`, `scanf` inside ISR body (heavy and non-reentrant) | HIGH |
| Missing volatile on shared variable | ISR and main thread share a variable without `volatile` | HIGH |
| Missing critical section | Read-modify-write on shared variable without IRQ disable/enable | HIGH |
| Direct register access outside HAL | Bare `0x4000xxxx` address used outside the HAL layer | HIGH |
| Blocking I/O without timeout | Blocking I/O API called with infinite-wait constant (`HAL_MAX_DELAY`, `UINT32_MAX`, `OS_WAIT_FOREVER`, `portMAX_DELAY`, `-1`, or equivalent) | HIGH |
| RTOS shared variable without synchronization | Variable accessed from multiple tasks without mutex, semaphore, or critical section (look for globals/statics written in one task and read in another) | HIGH |
| Large stack allocation | Local array or struct > 256 bytes on stack (adjust to project stack size if known from linker script) | MEDIUM |
| Magic register address | Bare hex constant for a peripheral register address | MEDIUM |

## Quality Checks

| Check | Threshold | Severity |
| ----- | --------- | -------- |
| memcpy without bounds check | `memcpy` used where the size argument is derived from a parameter without an explicit range check | HIGH |
| Function too long | > 50 lines | MEDIUM |
| Deep nesting | > 4 levels | MEDIUM |
| Unchecked return value | Return value of a function call discarded | HIGH |
| Magic number | Bare numeric literal (not 0 or 1) without named constant | LOW |
| Naming violation | Functions/variables not `snake_case`; macros not `UPPER_SNAKE_CASE` | LOW |
| Hidden global dependency | Function reads/writes a global without it being a parameter | MEDIUM |
| Missing error path | Function can fail internally but always returns success | HIGH |

## Output Format

For each issue, include severity, file location, small code snippet only when useful, reason, and concrete fix.

```markdown
# Code Review Report

**Date:** YYYY-MM-DD
**Diff:** `git diff HEAD~1 HEAD`
**Module(s):** uart.c, sensor.c

## Summary

<1-3 sentence overview of what changed and what areas to watch>

## Issues

### [CRITICAL] malloc called inside ISR

- **File:** `src/uart.c:87`
- **Reason:** Heap allocation in an ISR is unsafe because allocators are usually non-reentrant and may block.
- **Fix:** Use a static/file-scope buffer or preallocated pool.

### [HIGH] <title>
...

## Passed Checks

- [embedded] No ISR-only hazards found in changed functions
- [quality] No unchecked return values found in changed functions
```

## Inconclusive Checks

If a check cannot be determined from the diff alone (e.g., whether a variable is shared with an ISR), emit an `[INFO]` note rather than omitting the check:

```markdown
### [INFO] Cannot confirm volatile correctness
- **Symbol:** `rx_flag`
- **Note:** Cannot confirm from diff alone whether this variable is written by an ISR. Verify that no ISR accesses `rx_flag` outside this diff.
```

## Output File

Write to `docs/reviews/YYYY-MM-DD-<module>-review.md` relative to the repository root when a `docs/` directory already exists. `<module>` is the basename of the primary changed file; if multiple files changed, use the first alphabetically.

```bash
mkdir -p docs/reviews
```

If no `docs/` directory exists in the project root, print the report to stdout instead.

## Severity Reference

| Level    | Meaning                                          | Action                  |
| -------- | ------------------------------------------------ | ----------------------- |
| CRITICAL | Safety risk, data corruption, undefined behavior | Must fix before merge   |
| HIGH     | Bug or significant quality issue                 | Should fix before merge |
| MEDIUM   | Maintainability concern                          | Consider fixing         |
| LOW      | Style or minor suggestion                        | Optional                |
| INFO     | Cannot be determined from diff alone             | Verify manually outside diff |
