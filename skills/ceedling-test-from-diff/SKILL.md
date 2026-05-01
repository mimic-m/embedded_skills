---
name: ceedling-test-from-diff
description: Use when generating or updating Ceedling unit tests for C code from git diff output. Triggered by requests to write tests for changed C functions, create or update test_*.c files, or sync outdated tests to match current code.
---

# Ceedling Test Generation from Git Diff

## Overview

Parse `git diff` to identify changed C functions, then **create or update** Ceedling-compatible `test_<module>.c` files using Unity assertions and FFF fakes.

**For mock/fake generation, use `fff-mock-generation` skill alongside this one.**

## Workflow

```
git diff → check existing test file → identify changed functions → create/update test_<module>.c
```

### Step 0: Check for Existing Test File

Before generating anything, read the existing test file if it exists:

```bash
# check if test file already exists
ls test/test_<module>.c
```

**If it exists → UPDATE mode.** Read the full file to understand:
- Which functions are already tested (avoid duplicates)
- Existing `setUp`/`tearDown` patterns to follow
- Mock includes already present
- Naming conventions used in the file

**If it does not exist → CREATE mode.** Proceed to generate from scratch.

### Step 1: Read the Diff

```bash
git diff HEAD          # unstaged changes
git diff HEAD~1 HEAD   # last commit
git diff main...HEAD   # branch changes
```

### Step 2: Extract from Diff

From each `@@` hunk, identify:
- **New/changed functions** → need test cases
- **Added conditionals** (`if`/`else`/`switch`) → need branch coverage
- **New parameters or return values** → need boundary tests
- **Called external functions** → need FFF fakes (use `fff-mock-generation` skill)

### Step 3: Determine Fake Needs

Functions called from the module under test that live in other modules must be faked with FFF.
**REQUIRED SUB-SKILL:** Use `fff-mock-generation` for all fake declarations, setUp wiring, and fake assertions.

### Step 4: Create or Update Test File

In **UPDATE mode**, before adding new tests, fix any stale tests first (see "Updating Stale Tests" below).

## File & Naming Conventions

| Item | Convention |
|------|-----------|
| Test file | `test/test_<module>.c` |
| Test function | `void test_<func>_<scenario>(void)` |
| Setup | `void setUp(void)` |
| Teardown | `void tearDown(void)` |
| Fake declarations | `FAKE_VOID_FUNC` / `FAKE_VALUE_FUNC` via FFF |

## Test File Template

### Pattern A — standalone function fakes (most common)

Use when the module under test calls free functions in other modules.

```c
#include "unity.h"
#include "fff.h"
#include "fff_unity_helper.h"   /* TEST_ASSERT_CALLED / CALLED_IN_ORDER */
#include "<module_under_test>.h"

DEFINE_FFF_GLOBALS;

/* Fake declarations — see fff-mock-generation skill */
FAKE_VALUE_FUNC(int, dep_read,  uint8_t *, size_t);
FAKE_VOID_FUNC (dep_flush, ctx_t *);

void setUp(void) {
    RESET_FAKE(dep_read);
    RESET_FAKE(dep_flush);
    FFF_RESET_HISTORY();
}

void tearDown(void) { }

void test_process_calls_flush_then_read(void) {
    dep_read_fake.return_val = 4;

    int rc = process(&ctx);

    TEST_ASSERT_CALLED_IN_ORDER(0, dep_flush);   /* flush comes first */
    TEST_ASSERT_CALLED_IN_ORDER(1, dep_read);
    TEST_ASSERT_EQUAL_INT(4, rc);
}

void test_process_returns_error_on_null(void) {
    int rc = process(NULL);

    TEST_ASSERT_EQUAL_INT(ERROR_NULL, rc);
    TEST_ASSERT_NOT_CALLED(dep_read);            /* read must not be reached */
}
```

### Pattern B — struct with function pointers (e.g. driver / backend pattern)

Use when the module calls through a vtable or backend struct rather than
direct symbol references.  FFF fakes are assigned to the function pointer
fields in `setUp`.

```c
#include "unity.h"
#include "fff.h"
#include "fff_unity_helper.h"
#include "<module_under_test>.h"
#include "<module>-private.h"   /* for the backend_t / vtable struct */

DEFINE_FFF_GLOBALS;

/* Declare fakes matching each function-pointer signature */
FAKE_VALUE_FUNC(int,    stub_send,  ctx_t *, const uint8_t *, int);
FAKE_VALUE_FUNC(int,    stub_flush, ctx_t *);

/* Optional: stub implementation that actually fills a buffer or returns
   a realistic value — assign to fake.custom_fake in setUp */
static int impl_send(ctx_t *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    (void)buf;
    return len;              /* simulate "all bytes sent" */
}

static backend_t test_backend;
static ctx_t     test_ctx;

void setUp(void) {
    RESET_FAKE(stub_send);
    RESET_FAKE(stub_flush);
    FFF_RESET_HISTORY();

    /* Wire fakes into the vtable */
    memset(&test_backend, 0, sizeof(test_backend));
    test_backend.send  = stub_send;
    test_backend.flush = stub_flush;

    memset(&test_ctx, 0, sizeof(test_ctx));
    test_ctx.backend = &test_backend;

    /* Default behaviour: send returns length as if successful */
    stub_send_fake.custom_fake  = impl_send;
    stub_flush_fake.return_val  = 0;
}

void tearDown(void) { }

void test_send_frame_calls_backend_send_once(void) {
    uint8_t frame[] = { 0x01, 0x02 };

    send_frame(&test_ctx, frame, sizeof(frame));

    TEST_ASSERT_CALLED(stub_send);
}

void test_send_frame_response_byte_content(void) {
    /* Assert on the actual buffer passed to send */
    uint8_t frame[] = { 0xDE, 0xAD };
    send_frame(&test_ctx, frame, sizeof(frame));

    const uint8_t *sent = (const uint8_t *)stub_send_fake.arg1_val;
    TEST_ASSERT_NOT_NULL(sent);
    TEST_ASSERT_EQUAL_HEX8(0xDE, sent[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAD, sent[1]);
}

void test_send_frame_flush_before_send(void) {
    uint8_t frame[] = { 0x01 };
    send_frame(&test_ctx, frame, sizeof(frame));

    /* Global call history verifies ordering */
    TEST_ASSERT_CALLED_IN_ORDER(0, stub_flush);
    TEST_ASSERT_CALLED_IN_ORDER(1, stub_send);
}
```

### Choosing between Pattern A and Pattern B

| Signal in diff | Pattern |
|---|---|
| `dep_func(args)` — direct call, `dep_func` is in another `.c` file | A |
| `ctx->backend->send(args)` — call through function pointer | B |
| `obj->vtable->method(args)` — vtable / interface | B |
| Mix of both | Both — standalone fakes for free functions, pointer fakes for vtable |

### fff_unity_helper.h macros (Ceedling FFF plugin)

| Macro | Meaning |
|---|---|
| `TEST_ASSERT_CALLED(f)` | `f` was called exactly once |
| `TEST_ASSERT_NOT_CALLED(f)` | `f` was never called |
| `TEST_ASSERT_CALLED_TIMES(n, f)` | `f` was called exactly `n` times |
| `TEST_ASSERT_CALLED_IN_ORDER(i, f)` | `f` was the `i`-th fake call (0-indexed) |

## Unity Assert Quick Reference

| Assertion | Use for |
|-----------|---------|
| `TEST_ASSERT_EQUAL_INT(exp, act)` | integers |
| `TEST_ASSERT_EQUAL_UINT8(exp, act)` | uint8_t |
| `TEST_ASSERT_EQUAL_HEX8(exp, act)` | hex bytes |
| `TEST_ASSERT_EQUAL_STRING(exp, act)` | strings |
| `TEST_ASSERT_EQUAL_MEMORY(exp, act, len)` | byte arrays |
| `TEST_ASSERT_NULL(ptr)` | null pointer |
| `TEST_ASSERT_NOT_NULL(ptr)` | non-null |
| `TEST_ASSERT_TRUE(cond)` | boolean true |
| `TEST_ASSERT_FALSE(cond)` | boolean false |
| `TEST_FAIL_MESSAGE("msg")` | explicit fail |

## FFF Fake Patterns (quick reference)

For full FFF reference see the `fff-mock-generation` skill.

```c
/* Set return value */
dep_func_fake.return_val = VALUE;

/* Return a sequence */
int seq[] = { OK, ERR, OK };
SET_RETURN_SEQ(dep_func, seq, 3);

/* Custom callback */
dep_func_fake.custom_fake = my_impl;

/* Assert call count */
TEST_ASSERT_EQUAL_INT(1, dep_func_fake.call_count);

/* Assert argument of most recent call */
TEST_ASSERT_EQUAL_INT(expected_arg, dep_func_fake.arg0_val);
```

## Updating Stale Tests

When a test file already exists, the diff may have broken existing tests. Detect and fix them **before** adding new tests.

### Staleness Detection from Diff

| Diff pattern | Likely stale test symptom | Fix |
|---|---|---|
| Function signature changed (added/removed param) | `dep_fake.arg0_val` check uses wrong index / compile error | Update `FAKE_*_FUNC` macro args; fix call site |
| Return type changed | `TEST_ASSERT_EQUAL_INT` with wrong type macro | Change assertion macro |
| Return value semantics changed | Assertion expects old value | Update expected value |
| Function renamed | Old `test_<old_name>_*` still references old symbol | Rename test or remove |
| Function deleted | Test still calls removed function | Remove entire test function |
| New mandatory dependency added | No `FAKE_*_FUNC` declared → link error | Add `FAKE_*_FUNC`, `RESET_FAKE` in setUp, and `call_count` assertion |

### Update Workflow

```
1. Read existing test_<module>.c
2. For each existing test function:
   a. Does it call a function whose signature changed in the diff? → fix FAKE_*_FUNC args
   b. Does it assert a return value that the diff changed? → fix expected value
   c. Does it reference a function that was deleted? → remove test
3. Add FAKE_*_FUNC for any new dependencies; add RESET_FAKE in setUp
4. Append new test functions for newly added code paths
```

### Example: Signature Change → Stale Test Fix

**Diff shows:**
```diff
-int sensor_read(sensor_t *s) {
+int sensor_read(sensor_t *s, uint8_t *out) {
```

**Existing stale test:**
```c
/* STALE: missing second argument */
void test_sensor_read_returns_ok(void) {
    sensor_t s = { .channel = 0 };
    TEST_ASSERT_EQUAL_INT(SENSOR_OK, sensor_read(&s));  /* ← compile error */
}
```

**Fixed test:**
```c
/* hal_adc_read now called internally — add fake at top of file:   */
/* FAKE_VALUE_FUNC(uint8_t, hal_adc_read, uint8_t);                */

void test_sensor_read_returns_ok(void) {
    sensor_t s = { .channel = 0 };
    uint8_t out = 0;
    hal_adc_read_fake.return_val = 0x42;

    TEST_ASSERT_EQUAL_INT(SENSOR_OK, sensor_read(&s, &out));
    TEST_ASSERT_EQUAL_HEX8(0x42, out);
}
```

## Diff Analysis Checklist

When reading the diff, for each changed function ask:

**For new tests (CREATE and UPDATE mode):**
- [ ] What are the new code paths (new `if`/`else`/`switch`)? → one test per branch
- [ ] What are valid inputs? → happy path test
- [ ] What are invalid inputs (NULL, 0, UINT_MAX)? → boundary/error tests
- [ ] Which external functions are now called? → add mock expectations
- [ ] Are any global variables or hardware registers accessed? → verify or mock them
- [ ] Does a return value now differ? → update or add assertion

**For stale test detection (UPDATE mode only):**
- [ ] Did any function signature change? → find existing tests calling it, fix arguments
- [ ] Did any return value change? → find assertions on that value, update expected
- [ ] Was any function removed from the public header? → remove its test functions
- [ ] Was a new dependency added that must always be called? → add Expect to existing tests

## Example: Diff → Tests

**Diff shows:**
```diff
-int sensor_read(sensor_t *s) {
-    return s->raw;
+int sensor_read(sensor_t *s, uint8_t *out) {
+    if (s == NULL || out == NULL) return SENSOR_ERR_NULL;
+    *out = hal_adc_read(s->channel);
+    return SENSOR_OK;
 }
```

**Generated tests:**
```c
void test_sensor_read_returns_error_when_sensor_null(void) {
    uint8_t out;
    TEST_ASSERT_EQUAL_INT(SENSOR_ERR_NULL, sensor_read(NULL, &out));
}

void test_sensor_read_returns_error_when_out_null(void) {
    sensor_t s = { .channel = 0 };
    TEST_ASSERT_EQUAL_INT(SENSOR_ERR_NULL, sensor_read(&s, NULL));
}

void test_sensor_read_writes_adc_value_to_out(void) {
    sensor_t s = { .channel = 2 };
    uint8_t out = 0;
    hal_adc_read_ExpectAndReturn(2, 0xAB);

    TEST_ASSERT_EQUAL_INT(SENSOR_OK, sensor_read(&s, &out));
    TEST_ASSERT_EQUAL_HEX8(0xAB, out);
}
```

## Verification (run in this order after writing tests)

All three checks must pass before the work is complete.

### 1. Test Pass/Fail

```bash
ceedling test:all               # run all modules
ceedling test:test_<module>     # run single module
```

Success criterion: `0 failures, 0 errors` in the summary line.

### 2. Valgrind Memory Leak Check

Ceedling does not invoke valgrind automatically unless configured. Run it directly on the compiled test executable:

```bash
# build test binary first (no run)
ceedling test:test_<module> 2>/dev/null

# then run under valgrind
valgrind \
  --leak-check=full \
  --show-leak-kinds=all \
  --track-origins=yes \
  --error-exitcode=1 \
  build/test/out/test_<module>.out
```

To run valgrind over all modules at once:

```bash
for exe in build/test/out/test_*.out; do
  echo "=== $exe ==="
  valgrind --leak-check=full --error-exitcode=1 "$exe" || exit 1
done
```

Success criterion: `All heap blocks were freed -- no leaks are possible` and exit code 0.

**Common valgrind errors and fixes:**

| Error | Cause | Fix |
|-------|-------|-----|
| `definitely lost: N bytes` | malloc without free in production code | Fix source; add `free()` |
| `still reachable: N bytes` | Global/static allocation never freed | Add cleanup in `tearDown` or accept if intentional static init |
| `Invalid read/write of size N` | Out-of-bounds access | Fix buffer sizing in source |
| `Uninitialised value` | Variable used before assignment | Initialize in `setUp` or production code |

**Note on FFF:** FFF fakes allocate no heap memory themselves, so valgrind leaks in tests are caused by the module under test or by `malloc` in custom fakes — not by FFF itself.

### 3. C0 Coverage (Statement Coverage) 100%

C0 = every statement in the module under test must be executed by at least one test.

**Enable gcov in project.yml** (if not already set):

```yaml
:plugins:
  :enabled:
    - gcov

:gcov:
  :utilities:
    - gcovr
  :reports:
    - HtmlDetailed
    - Text
```

**Run and check:**

```bash
ceedling gcov:test_<module>        # instrument and run
ceedling utils:gcov                # generate text report
```

Or with gcovr directly for a strict check:

```bash
ceedling gcov:test_<module> 2>/dev/null
gcovr --root . \
      --filter src/<module>.c \
      --fail-under-line 100 \
      --print-summary
```

Success criterion: exit code 0 from gcovr (`--fail-under-line 100`).

**When coverage is below 100%, add tests for the uncovered lines:**

```bash
# see exactly which lines are not covered
gcovr --root . --filter src/<module>.c --branches --html-details coverage.html
# open coverage.html and look for red-highlighted lines
```

Each uncovered line → add a test that exercises that path. Re-run gcovr until 100%.

**Common reasons for missing C0 coverage:**

| Uncovered line type | Fix |
|---------------------|-----|
| Error branch (`if (ptr == NULL)`) | Add a test that passes NULL |
| Default case in `switch` | Add a test with an out-of-range enum value |
| Loop body never entered | Add a test with zero-length / empty input |
| `assert()` macro lines | Compile with `NDEBUG` or mock the assert |

### Verification Checklist

After writing or updating tests:

- [ ] `ceedling test:all` → 0 failures, 0 errors
- [ ] `valgrind` on each test binary → no leaks, exit 0
- [ ] `gcovr --fail-under-line 100` → exit 0 for each changed module

## Common Mistakes

| Mistake | Fix |
|---------|-----|
| No `FAKE_*_FUNC` for a called dependency | Declare fake and add `RESET_FAKE` in setUp |
| Forgetting `RESET_FAKE` in setUp | State leaks between tests; always reset in setUp, not tearDown |
| Checking `arg0_val` before checking `call_count` | Assert call_count first; arg values are undefined if not called |
| One test covering multiple branches | Split into one test per branch |
| Verifying too many internal details | Test behavior (return value, outputs), not implementation |
| Including implementation `.c` directly | Include only the public header |
