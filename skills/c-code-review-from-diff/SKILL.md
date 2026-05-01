---
name: c-code-review-from-diff
description: Use when reviewing C/C++ embedded code changes from git diff. Triggered by requests to review changed C/C++ functions, check embedded-specific issues (interrupt safety, memory management, HAL boundaries), or produce a severity-annotated code review report.
---

# C/C++ Embedded Code Review from Git Diff

## Overview

Parse `git diff` to identify changed C/C++ functions, then produce a severity-annotated Markdown report covering embedded-specific and general quality concerns.

## Workflow

```text
git diff → parse diff → extract changed functions → embedded checks → quality checks → output report
```

### Step 0: Get the Diff

```bash
git diff HEAD          # unstaged changes
git diff HEAD~1 HEAD   # last commit
git diff main...HEAD   # branch changes
```

### Step 1: Extract Changed Functions

From each `@@` hunk, identify:

- New or modified functions
- Changed function signatures
- New conditionals (`if`/`else`/`switch`) — note for branch coverage
- New external calls — potential HAL boundary violations

### Step 2: Embedded-Specific Checks

For each changed function, apply the following checks:

| Check | Pattern to detect | Severity |
| ----- | ----------------- | -------- |
| malloc/free in ISR | `malloc`, `free`, `new`, `delete` inside ISR handler | CRITICAL |
| Blocking call in ISR | `HAL_Delay`, `osDelay`, blocking I/O inside ISR body | CRITICAL |
| Missing volatile on shared variable | ISR and main thread share a variable without `volatile` | HIGH |
| Missing critical section | Read-modify-write on shared variable without IRQ disable/enable | HIGH |
| Direct register access outside HAL | Bare `0x4000xxxx` address used outside the HAL layer | HIGH |
| Blocking I/O without timeout | `HAL_UART_Receive`, `HAL_SPI_TransmitReceive`, etc. with `HAL_MAX_DELAY` | HIGH |
| Large stack allocation | Local array or struct > 256 bytes on stack | MEDIUM |
| Magic register address | Bare hex constant for a peripheral register address | MEDIUM |

### Step 3: General Quality Checks

| Check | Threshold | Severity |
| ----- | --------- | -------- |
| Function too long | > 50 lines | MEDIUM |
| Deep nesting | > 4 levels | MEDIUM |
| Unchecked return value | Return value of a function call discarded | HIGH |
| Magic number | Bare numeric literal (not 0 or 1) without named constant | LOW |
| Naming violation | Functions/variables not `snake_case`; macros not `UPPER_SNAKE_CASE` | LOW |
| Hidden global dependency | Function reads/writes a global without it being a parameter | MEDIUM |
| Missing error path | Function can fail internally but always returns success | HIGH |

### Step 4: Write the Report

For each issue found, write a block with severity tag, file location, code snippet, reason, and concrete fix.

## Output Format

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
- **Code:**
```c
void USART1_IRQHandler(void) {
    char *buf = malloc(RX_BUF_LEN);  /* heap allocation in ISR */
}
```
- **Reason:** Heap allocation in an ISR is unsafe — the allocator is not reentrant and may deadlock if the ISR fires during a malloc in main context.
- **Fix:** Declare the buffer as a static or file-scope array instead.

```c
static char rx_buf[RX_BUF_LEN];
void USART1_IRQHandler(void) {
    char *buf = rx_buf;
}
```

### [HIGH] <title>
...

## Passed Checks

- [embedded] No malloc/free in ISR
- [embedded] No blocking calls in ISR
- [embedded] All shared variables are volatile
- [quality ] All return values are checked
- [quality ] No functions exceed 50 lines
```

## Output File

Write to `docs/reviews/YYYY-MM-DD-<module>-review.md`. Create the directory if it does not exist:

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

## Common Embedded Fix Patterns

### ISR-safe variable sharing

```c
/* WRONG — missing volatile */
static uint8_t rx_flag = 0;
void USART1_IRQHandler(void) { rx_flag = 1; }

/* CORRECT */
static volatile uint8_t rx_flag = 0;
void USART1_IRQHandler(void) { rx_flag = 1; }
```

### Critical section for read-modify-write

```c
/* WRONG — non-atomic on Cortex-M */
shared_counter++;

/* CORRECT */
__disable_irq();
shared_counter++;
__enable_irq();
```

### Timeout on blocking I/O

```c
/* WRONG — infinite block */
HAL_UART_Receive(&huart1, buf, len, HAL_MAX_DELAY);

/* CORRECT */
HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, buf, len, UART_TIMEOUT_MS);
if (status != HAL_OK) {
    /* handle timeout or error */
}
```
