---
name: change-spec-from-diff
description: Use when generating a change specification document from git diff output. Triggered by requests to document what changed, write release notes for embedded C/C++ code, or produce a traceability record of code modifications.
---

# Change Specification from Git Diff

## Overview

Parse `git diff` to classify changes by module and function, then generate a structured Markdown change specification document suitable for handoff, release notes, or traceability.

## Workflow

```text
git diff → classify changes by module/function → determine change type → expand into template → write Markdown
```

### Step 1: Get the Diff and Git Metadata

```bash
# Get the diff (adjust ref as needed)
git diff HEAD~1 HEAD

# Other common ranges:
git diff <tag>..HEAD          # since a release tag
git diff origin/main...HEAD   # all branch commits

# Get commit metadata for the spec header
git log -1 --format="%H %an %ad %s" --date=short
git branch --show-current
```

### Step 2: Classify Changes

For each changed file, group hunks by function and classify using this table:

| Diff pattern | Change type |
| ------------ | ----------- |
| New function added (`+<return_type> <name>(`) | New feature |
| Function signature changed (params or return type differ) | Interface change |
| Function body modified, signature unchanged | Behavior change |
| Function deleted (`-<return_type> <name>(`) | Feature removal |
| `#define` value changed | Constant change |
| New `.c` or `.h` file added | New module |
| `.c` or `.h` file deleted | Module removal |

**When the diff touches multiple files:** Create one `### <module>` sub-section per changed module in the output. If more than 5 modules change, split into one spec file per module and link them from a summary document.

**For header-only changes** (struct layout, typedef, macro value): use the before/after table with rows `Declaration`, `Effective size/value`, and `Impact` instead of the default Signature/Behavior/Return rows.

### Step 3: Determine Impact

For each changed function or constant, find callers and test files:

```bash
# Find callers in source and include directories
grep -rn "\bfunction_name\b" src/ include/

# Find related test files
grep -rn "\bfunction_name\b" test/
```

Record: caller file paths and line numbers, test file paths.

### Step 4: Write the Specification

Use the template below. Fill every field — do not leave any section blank.

## Output Template

```markdown
# Change Specification

**Date:** YYYY-MM-DD
**Commit:** abc1234 (short description of change)
**Author:** First Last
**Branch:** feature/uart-timeout

---

## Summary

<1-3 sentence plain-language description of what changed and why>

---

## Changed Modules

### `uart` (`src/uart.c`)

#### `uart_send`

|           | Before                        | After                              |
| --------- | ----------------------------- | ---------------------------------- |
| Signature | `int uart_send(uint8_t *buf, int len)` | `int uart_send(uint8_t *buf, size_t len, uint32_t timeout_ms)` |
| Behavior  | Blocks until all bytes sent   | Returns `UART_ERR_TIMEOUT` if not done within `timeout_ms` |
| Return    | `0` on success                | `0` on success, `UART_ERR_TIMEOUT` on timeout |

**Change type:** Interface change

**Impact:** All callers must pass the new `timeout_ms` argument.
Affected call sites: `src/protocol.c:45`, `src/logger.c:112`

---

## Compatibility Notes

> **Breaking change:** `uart_send()` now requires a third argument `timeout_ms`.
> Update all call sites. Recommended value: `UART_DEFAULT_TIMEOUT_MS` (defined in `uart.h`).

> For non-breaking changes, write: `No breaking changes.`

---

## Related Tests

| Test file | Action required |
| --------- | --------------- |
| `test/test_uart.c` | Update — signature changed, add timeout test cases |
| `test/test_protocol.c` | Update — call site passes new argument |
```

## Output File

Write to `docs/changes/YYYY-MM-DD-<module>-change-spec.md` where `<module>` is the basename of the primary changed file. Create the directory if it does not exist:

```bash
mkdir -p docs/changes
```
