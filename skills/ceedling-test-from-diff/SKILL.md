---
name: ceedling-test-from-diff
description: Use when generating or updating Ceedling unit tests and FFF fakes for C code from git diff output. Triggered by requests to write tests for changed C functions, create or update test_*.c files, generate FAKE_VOID_FUNC/FAKE_VALUE_FUNC declarations, or sync outdated tests to match current code.
---

# Ceedling Test Generation from Git Diff

## Overview

Parse `git diff` to identify changed C functions, then **create or update** Ceedling-compatible `test_<module>.c` files using Unity assertions and FFF fakes.

## Workflow

```
git diff → check existing test file → identify changed functions/dependencies → create/update test_<module>.c with FFF fakes
```

### Step 0: Check for Existing Test File

Before generating anything, run the full test suite once to establish a baseline. This ensures any pre-existing failures are not confused with newly introduced ones:

```bash
ceedling test:all   # establish baseline — note any pre-existing failures
```

If the baseline has failures unrelated to the diff, note them but do not fix them now. Proceed to test generation; only the new/changed functions are in scope.

Next, read the existing test file if it exists:

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
- **Called external functions** → need FFF fakes

### Step 3: Determine Fake Needs

Functions called from the module under test that live in other modules must be faked with FFF. Declare fakes in the test file, reset them in `setUp`, configure return values or callbacks per test, and assert call counts/arguments when behavior depends on the dependency interaction.

### Step 4: Create or Update Test File

In **UPDATE mode**, before adding new tests, fix any stale tests first (see "Updating Stale Tests" below).

## File & Naming Conventions

| Item | Convention |
|------|-----------|
| Test file | `test/test_<module>.c` |
| Test function | `void test_<target_function>_<Success\|Fail>_<representative_condition>(void)` |
| Setup | `void setUp(void)` |
| Teardown | `void tearDown(void)` |
| Fake declarations | `FAKE_VOID_FUNC` / `FAKE_VALUE_FUNC` via FFF |

### Test Function Naming for Reports

Generated tests must use this function name format:

```c
void test_<target_function>_<Success|Fail>_<representative_condition>(void)
```

Parsing rules:
- The test target function is the text after `test_` and before the first `_Success_` or `_Fail_`.
- `Success` means the expected normal path; `Fail` means the expected error path.
- The representative condition summarizes the main test parameters and mock return values, using lowercase words separated by `_`.
- If the production function name contains `_`, keep it unchanged. The `_Success_` or `_Fail_` token is the delimiter.
- Do not use production function names that contain `_Success_` or `_Fail_` in generated tests. If the real function name contains either token, add a report-safe wrapper naming note and choose an unambiguous test target alias for the report.
- In Excel, convert the representative condition suffix from `_` separators to spaces. Example: `valid_ctx_dep_read_returns_4` becomes `valid ctx dep read returns 4`.

Examples:

```c
void test_process_Success_valid_ctx_dep_read_returns_4(void)
void test_process_Fail_null_ctx(void)
void test_module_process_request_Success_valid_id_dep_read_ok(void)
```

### Excel Report Extraction Markers

Every generated test case must include all three marker pairs below. The Excel report script extracts the real C code between each start marker and end marker and writes it to the corresponding Excel column.

Use the Japanese labels exactly as shown. Missing or misspelled markers are extraction errors.

Parser boundary rule:
- A start marker is the full Doxygen comment block that contains `@verbatim <label>` and `@endverbatim`.
- An end marker is the full Doxygen comment block that contains `@verbatim <label>終了` and `@endverbatim`.
- Extract only the C source text after the closing `*/` of the start marker and before the opening `/**` of the end marker.
- Do not include the marker comments themselves in Excel cells.
- Preserve the code text inside the extracted range except for trimming leading and trailing blank lines.
- Empty ranges are invalid. A range containing only C comments is allowed only when no executable setup is needed, such as a NULL argument passed directly in the sequence block.

```c
/** @verbatim テストパラメータ
 * @endverbatim
 */
/* Test parameter setup code goes here. */
/** @verbatim テストパラメータ終了
 * @endverbatim
 */

/** @verbatim テストシーケンス
 * @endverbatim
 */
/* Function call or execution sequence code goes here. */
/** @verbatim テストシーケンス終了
 * @endverbatim
 */

/** @verbatim チェック項目
 * @endverbatim
 */
/* Unity assertions and FFF call checks go here. */
/** @verbatim チェック項目終了
 * @endverbatim
 */
```

Extraction mapping:
- `テストパラメータ` to `テストパラメータ終了` → test parameters
- `テストシーケンス` to `テストシーケンス終了` → test sequence
- `チェック項目` to `チェック項目終了` → check items

## Test File Template

### Pattern A — standalone function fakes (most common)

Use when the module under test calls free functions in other modules.

```c
#include "unity.h"
#include "fff.h"
#include "fff_unity_helper.h"   /* TEST_ASSERT_CALLED / CALLED_IN_ORDER */
#include "<module_under_test>.h"

DEFINE_FFF_GLOBALS;

/* Fake declarations */
FAKE_VALUE_FUNC(int, dep_read,  uint8_t *, size_t);
FAKE_VOID_FUNC (dep_flush, ctx_t *);

void setUp(void) {
    RESET_FAKE(dep_read);
    RESET_FAKE(dep_flush);
    FFF_RESET_HISTORY();
}

void tearDown(void) { }

void test_process_Success_valid_ctx_dep_read_returns_4(void) {
    /** @verbatim テストパラメータ
     * @endverbatim
     */
    int rc;
    dep_read_fake.return_val = 4;
    /** @verbatim テストパラメータ終了
     * @endverbatim
     */

    /** @verbatim テストシーケンス
     * @endverbatim
     */
    rc = process(&ctx);
    /** @verbatim テストシーケンス終了
     * @endverbatim
     */

    /** @verbatim チェック項目
     * @endverbatim
     */
    TEST_ASSERT_CALLED_IN_ORDER(0, dep_flush);   /* flush comes first */
    TEST_ASSERT_CALLED_IN_ORDER(1, dep_read);
    TEST_ASSERT_EQUAL_INT(4, rc);
    /** @verbatim チェック項目終了
     * @endverbatim
     */
}

void test_process_Fail_null_ctx(void) {
    /** @verbatim テストパラメータ
     * @endverbatim
     */
    int rc;
    /** @verbatim テストパラメータ終了
     * @endverbatim
     */

    /** @verbatim テストシーケンス
     * @endverbatim
     */
    rc = process(NULL);
    /** @verbatim テストシーケンス終了
     * @endverbatim
     */

    /** @verbatim チェック項目
     * @endverbatim
     */
    TEST_ASSERT_EQUAL_INT(ERROR_NULL, rc);
    TEST_ASSERT_NOT_CALLED(dep_read);            /* read must not be reached */
    /** @verbatim チェック項目終了
     * @endverbatim
     */
}
```

### Pattern B — struct with function pointers (e.g. driver / backend pattern)

Use when the module calls through a vtable or backend struct rather than
direct symbol references.  FFF fakes are assigned to the function pointer
fields in `setUp`.

```c
FAKE_VALUE_FUNC(int,    stub_send,  ctx_t *, const uint8_t *, int);
FAKE_VALUE_FUNC(int,    stub_flush, ctx_t *);

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
}
```

### Pattern C — RTOS API fakes (FreeRTOS / CMSIS-RTOS)

Use when the module under test calls RTOS primitives directly (`xQueueSend`, `xTaskNotify`, `osSemaphoreAcquire`, etc.). Fake the RTOS API rather than pulling in the real scheduler.

```c
FAKE_VALUE_FUNC(BaseType_t, xQueueSend,    QueueHandle_t, const void *, TickType_t);
FAKE_VALUE_FUNC(BaseType_t, xQueueReceive, QueueHandle_t, void *,       TickType_t);
FAKE_VALUE_FUNC(BaseType_t, xTaskNotify, TaskHandle_t, uint32_t, eNotifyAction);
FAKE_VALUE_FUNC(BaseType_t, xSemaphoreTake, SemaphoreHandle_t, TickType_t);
FAKE_VALUE_FUNC(BaseType_t, xSemaphoreGive, SemaphoreHandle_t);

void setUp(void) {
    RESET_FAKE(xQueueSend);
    RESET_FAKE(xQueueReceive);
    RESET_FAKE(xTaskNotify);
    RESET_FAKE(xSemaphoreTake);
    RESET_FAKE(xSemaphoreGive);
    FFF_RESET_HISTORY();

    xSemaphoreTake_fake.return_val = pdTRUE;   /* default: lock succeeds */
    xSemaphoreGive_fake.return_val = pdTRUE;
}
```

> **Note on FreeRTOS headers:** Include `FreeRTOS.h` only for type definitions. Do not link the real FreeRTOS kernel — the fakes replace all scheduler calls. Add `-DUNIT_TEST` or equivalent to suppress any `configASSERT` macros that would call real RTOS functions.

### Choosing between Pattern A, Pattern B, and Pattern C

| Signal in diff | Pattern |
|---|---|
| `dep_func(args)` — direct call, `dep_func` is in another `.c` file | A |
| `ctx->backend->send(args)` — call through function pointer | B |
| `obj->vtable->method(args)` — vtable / interface | B |
| `xQueueSend`, `xTaskNotify`, `osSemaphore*`, etc. — RTOS primitive | C |
| Mix of free functions + vtable | A + B |
| Mix of free functions + RTOS | A + C |

### fff_unity_helper.h macros (Ceedling FFF plugin)

`fff_unity_helper.h` is provided by the Ceedling FFF plugin or by a project-owned test support file. Before including it in a generated test, confirm that the project already provides it or add it under `test/support`. If it is unavailable, use raw `*_fake.call_count` and `fff.call_history` assertions consistently instead of including a missing header.

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

FFF requires no Ceedling plugin. Add `fff.h` to the include path, include it in the test file, and declare exactly one `DEFINE_FFF_GLOBALS;` per test translation unit. Never put `DEFINE_FFF_GLOBALS;` in a header.

### Fake Declaration Decision Table

| Dependency signature | Macro |
|---|---|
| `void dep(void)` | `FAKE_VOID_FUNC(dep)` |
| `void dep(T1, T2)` | `FAKE_VOID_FUNC(dep, T1, T2)` |
| `T dep(void)` | `FAKE_VALUE_FUNC(T, dep)` |
| `T dep(T1, T2)` | `FAKE_VALUE_FUNC(T, dep, T1, T2)` |
| `void dep(T1, ...)` | `FAKE_VOID_FUNC_VARARG(dep, T1, ...)` |
| `T dep(T1, ...)` | `FAKE_VALUE_FUNC_VARARG(T, dep, T1, ...)` |
| Removed dependency call | Remove the `FAKE_*` declaration, `RESET_FAKE`, and related assertions |
| Changed dependency signature | Update the `FAKE_*` macro arguments and affected argument assertions |

### Ceedling project.yml Setup

```yaml
:paths:
  :include:
    - test/support     # place fff.h here

:plugins:
  :enabled:
    - fff

:defines:
  :test:
    - FFF_ARG_HISTORY_LEN=20
```

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
| Existing test lacks Excel markers | Report script fails in UPDATE mode | Add all three Japanese marker pairs while touching the test |
| Existing test name lacks `_Success_` or `_Fail_` | Report script rejects the test case | Rename to `test_<target>_<Success|Fail>_<representative_condition>` |

### Update Workflow

```
1. Read existing test_<module>.c
2. For each existing test function:
   a. Does it call a function whose signature changed in the diff? → fix FAKE_*_FUNC args
   b. Does it assert a return value that the diff changed? → fix expected value
   c. Does it reference a function that was deleted? → remove test
   d. Does it lack the required `_Success_` or `_Fail_` delimiter? → rename it
   e. Does it lack any Excel extraction marker pair? → add all three marker pairs
3. Add FAKE_*_FUNC for any new dependencies; add RESET_FAKE in setUp
4. Append new test functions for newly added code paths
5. Before reporting, ensure every test case in each reported test file follows the naming and marker rules, including legacy tests not directly changed by the diff
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

void test_sensor_read_Success_channel_0_hal_adc_read_returns_42(void) {
    /** @verbatim テストパラメータ
     * @endverbatim
     */
    sensor_t s = { .channel = 0 };
    uint8_t out = 0;
    int rc;
    hal_adc_read_fake.return_val = 0x42;
    /** @verbatim テストパラメータ終了
     * @endverbatim
     */

    /** @verbatim テストシーケンス
     * @endverbatim
     */
    rc = sensor_read(&s, &out);
    /** @verbatim テストシーケンス終了
     * @endverbatim
     */

    /** @verbatim チェック項目
     * @endverbatim
     */
    TEST_ASSERT_EQUAL_INT(SENSOR_OK, rc);
    TEST_ASSERT_EQUAL_HEX8(0x42, out);
    TEST_ASSERT_CALLED(hal_adc_read);
    /** @verbatim チェック項目終了
     * @endverbatim
     */
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
void test_sensor_read_Fail_sensor_null(void) {
    /** @verbatim テストパラメータ
     * @endverbatim
     */
    uint8_t out;
    int rc;
    /** @verbatim テストパラメータ終了
     * @endverbatim
     */

    /** @verbatim テストシーケンス
     * @endverbatim
     */
    rc = sensor_read(NULL, &out);
    /** @verbatim テストシーケンス終了
     * @endverbatim
     */

    /** @verbatim チェック項目
     * @endverbatim
     */
    TEST_ASSERT_EQUAL_INT(SENSOR_ERR_NULL, rc);
    TEST_ASSERT_NOT_CALLED(hal_adc_read);
    /** @verbatim チェック項目終了
     * @endverbatim
     */
}

void test_sensor_read_Fail_out_null(void) {
    /** @verbatim テストパラメータ
     * @endverbatim
     */
    sensor_t s = { .channel = 0 };
    int rc;
    /** @verbatim テストパラメータ終了
     * @endverbatim
     */

    /** @verbatim テストシーケンス
     * @endverbatim
     */
    rc = sensor_read(&s, NULL);
    /** @verbatim テストシーケンス終了
     * @endverbatim
     */

    /** @verbatim チェック項目
     * @endverbatim
     */
    TEST_ASSERT_EQUAL_INT(SENSOR_ERR_NULL, rc);
    TEST_ASSERT_NOT_CALLED(hal_adc_read);
    /** @verbatim チェック項目終了
     * @endverbatim
     */
}

void test_sensor_read_Success_channel_2_hal_adc_read_returns_ab(void) {
    /** @verbatim テストパラメータ
     * @endverbatim
     */
    sensor_t s = { .channel = 2 };
    uint8_t out = 0;
    int rc;
    hal_adc_read_fake.return_val = 0xAB;
    /** @verbatim テストパラメータ終了
     * @endverbatim
     */

    /** @verbatim テストシーケンス
     * @endverbatim
     */
    rc = sensor_read(&s, &out);
    /** @verbatim テストシーケンス終了
     * @endverbatim
     */

    /** @verbatim チェック項目
     * @endverbatim
     */
    TEST_ASSERT_EQUAL_INT(SENSOR_OK, rc);
    TEST_ASSERT_EQUAL_HEX8(0xAB, out);
    TEST_ASSERT_CALLED(hal_adc_read);
    TEST_ASSERT_EQUAL_INT(2, hal_adc_read_fake.arg0_val);   /* channel passed through */
    /** @verbatim チェック項目終了
     * @endverbatim
     */
}
```

## Excel Unit Test Report Support

When asked to produce test evidence, generate tests so a post-processing report script can create an Excel workbook after Ceedling execution.

For the detailed `tools/ceedling_report_excel.py` implementation contract, use `templates/ceedling_report_excel_script_spec.md`.

### Scope Modes

Default mode:
- Use `git diff` to find changed production `.c` files.
- Assume a 1:1 mapping between source and test file: `src/<module>.c` ↔ `test/test_<module>.c`.
- A diff-target test file means the test file corresponding to a changed production `.c` file, not only a test file that appears in the diff.
- Run and report all test cases in each diff-target test file.

Regression mode:
- Run `ceedling test:all`.
- Report all `test_*.c` files and all test cases.
- Signal this mode with `--mode regression`. The default is `--mode diff`.

### Recommended Script Interface

Keep Excel generation as a post-processing script instead of embedding it into Ceedling:

```bash
python tools/ceedling_report_excel.py \
  --mode diff \
  --template templates/unit_test_report_template.xlsx \
  --junit build/artifacts/test/report.xml \
  --coverage build/reports/gcovr.json \
  --coverage-reasons coverage_uncovered_reasons.yml \
  --memory build/reports/valgrind \
  --out build/reports/unit_test_report.xlsx
```

Recommended implementation:
- Use Python.
- Use `openpyxl` for workbook generation.
- Use JUnit XML as the only source for test pass/fail results.
- Use gcovr JSON for C0 and C1 coverage results.
- Use Valgrind Memcheck XML for memory leak results. Accept either one XML file or a directory containing one XML file per test binary.
- In `--mode diff`, derive source files from `git diff` and map `src/<module>.c` to `test/test_<module>.c`.
- In `--mode regression`, scan all `test/test_*.c` files.
- Read an existing `.xlsx` template when provided; otherwise create a minimal workbook with the required sheets.
- Create `unit_test_evidence.zip` beside the workbook and list it in the summary sheet.
- Do not use Windows Excel COM automation. If `--embed-ole` is requested, use a pure-Python/template-based OOXML approach: the workbook template must already contain a placeholder embedded OLE Package object, and the script replaces that object's package payload with `unit_test_evidence.zip`. If no placeholder exists, fail with a clear error and still leave the evidence zip beside the workbook.

### Workbook Sheets

The workbook must contain these sheets:
- 表紙
- 変更履歴
- テスト方針
- テスト結果サマリ
- 各テストケースの情報
- カバレッジ結果
- メモリリークチェック結果

### Test Result Summary Sheet

Include:
- テスト対象
- テスト実施日
- Overall test result from JUnit XML
- Overall coverage result from gcovr JSON
- Overall memory leak result from Valgrind XML
- Evidence zip filename, timestamp, and included file list

Recommended evidence zip contents:
- JUnit XML
- gcovr JSON
- Valgrind XML file or XML directory
- `coverage_uncovered_reasons.yml` when present
- Optional raw logs or HTML reports as evidence only, not as parser inputs

### Per-Test-Case Sheet

For each test case, output:
- ソースコード (source file path, for example `src/foo.c`)
- テスト関数 (production function under test, parsed from the test case function name)
- テストケース関数名
- 正常系/異常系情報 (`Success` or `Fail`, parsed from function name)
- 代表的なテストパラメータ及びmock関数の戻り値 (parsed from the representative condition in the function name, with `_` converted to spaces)
- テストパラメータ (code between `テストパラメータ` and `テストパラメータ終了`)
- テストシーケンス (code between `テストシーケンス` and `テストシーケンス終了`)
- チェック項目 (code between `チェック項目` and `チェック項目終了`)
- テスト結果 (`Success` or `Fail`, from JUnit XML)

The report script should fail if a test case is missing any required marker pair, violates the naming convention, or has no matching JUnit XML result.

For C89/C90 embedded projects, keep declarations before statements inside each test function. Put local variable declarations in the `テストパラメータ` block before fake return assignments or other executable statements.

### Coverage Sheet

Include a coverage summary table:
- ファイル
- C0
- C1
- 行数
- カバー行数
- 分岐数
- カバー分岐数

Also include an uncovered-location table:
- No
- ファイル
- 関数
- 種別 (`C0` or `C1`)
- 行番号
- 非通過コードまたは条件
- 非通過理由
- 対応方針

The report script should extract uncovered lines and branches from gcovr JSON. `coverage_uncovered_reasons.yml` may fill `非通過理由` and `対応方針`, but missing reasons must not fail workbook generation because they may be entered later.

If a reason entry does not match any current uncovered item, warn about the stale entry but continue. Line-number based entries can become stale after refactoring.

Example reason file:

```yaml
uncovered:
  - file: src/foo.c
    function: process
    type: C1
    line: 42
    reason: "防御的分岐であり、通常API経由では到達不可"
    action: "レビューにより妥当性確認"
```

### Memory Leak Sheet

Parse Valgrind Memcheck XML and output:
- テスト対象
- 種別
- 結果
- サイズ
- 件数
- 発生箇所
- 詳細

## Verification (run in this order after writing tests)

All three checks must pass before the work is complete.

### 1. Test Pass/Fail

```bash
ceedling test:all               # run all modules
ceedling test:test_<module>     # run single module
```

Success criterion: `0 failures, 0 errors` in the summary line.

### 2. Valgrind Memory Leak Check

Ceedling does not invoke valgrind automatically unless configured. Run it directly on the compiled test executable and always generate XML for the Excel report:

```bash
# build test binary first (no run)
ceedling test:test_<module> 2>/dev/null

# then run under valgrind
mkdir -p build/reports/valgrind
valgrind \
  --leak-check=full \
  --show-leak-kinds=all \
  --track-origins=yes \
  --xml=yes \
  --xml-file=build/reports/valgrind/test_<module>.xml \
  --error-exitcode=1 \
  build/test/out/test_<module>.out
```

To run valgrind over all modules at once:

```bash
mkdir -p build/reports/valgrind
for exe in build/test/out/test_*.out; do
  name=$(basename "$exe" .out)
  valgrind \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --xml=yes \
    --xml-file="build/reports/valgrind/${name}.xml" \
    --error-exitcode=1 \
    "$exe" || exit 1
done
```

Success criterion: exit code 0 and a Valgrind XML file for each executed test binary. If text output is also captured, `All heap blocks were freed -- no leaks are possible` is the expected no-leak summary.

**Common valgrind errors and fixes:**

| Error | Cause | Fix |
|-------|-------|-----|
| `definitely lost: N bytes` | malloc without free in production code | Fix source; add `free()` |
| `still reachable: N bytes` | Global/static allocation never freed | Add cleanup in `tearDown` or accept if intentional static init |
| `Invalid read/write of size N` | Out-of-bounds access | Fix buffer sizing in source |
| `Uninitialised value` | Variable used before assignment | Initialize in `setUp` or production code |

**Note on FFF:** FFF fakes allocate no heap memory themselves, so valgrind leaks in tests are caused by the module under test or by `malloc` in custom fakes — not by FFF itself.

### 3. C0/C1 Coverage

C0 = every statement in the module under test must be executed by at least one test.
C1 = every branch in the module under test must be exercised by at least one test.

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
    - JSON
```

**Run and check:**

```bash
ceedling gcov:test_<module>        # instrument and run
ceedling utils:gcov                # generate configured reports, including JSON when supported
```

Or with gcovr directly for a strict check:

```bash
ceedling gcov:test_<module> 2>/dev/null
mkdir -p build/reports
gcovr --root . \
      --filter src/<module>.c \
      --branches \
      --json build/reports/gcovr.json \
      --fail-under-line 100 \
      --fail-under-branch 100 \
      --print-summary
```

Success criterion: exit code 0 from gcovr (`--fail-under-line 100` and `--fail-under-branch 100`) and `build/reports/gcovr.json` exists.

**When coverage is below 100%, add tests for the uncovered lines:**

```bash
# see exactly which lines are not covered
gcovr --root . --filter src/<module>.c --branches --html-details coverage.html
# open coverage.html and look for red-highlighted lines
```

Each uncovered line or branch → add a test that exercises that path, or document the uncovered item in the coverage non-passage table if it is intentionally unreachable. Re-run gcovr until the required threshold is met.

**Common reasons for missing C0 coverage:**

| Uncovered line type | Fix |
|---------------------|-----|
| Error branch (`if (ptr == NULL)`) | Add a test that passes NULL |
| Default case in `switch` | Add a test with an out-of-range enum value |
| Loop body never entered | Add a test with zero-length / empty input |
| `assert()` macro lines | Compile with `NDEBUG` or mock the assert |

**Common reasons for missing C1 coverage:**

| Uncovered branch type | Fix |
|-----------------------|-----|
| `if` true or false side not taken | Add a test for the missing side |
| `switch` case not taken | Add a test for that enum or value |
| Short-circuit `&&` / `||` operand not evaluated both ways | Add tests that force each operand outcome |
| Defensive branch intentionally unreachable | Record the non-passage reason and review it |

### Verification Checklist

After writing or updating tests:

- [ ] `ceedling test:all` → 0 failures, 0 errors
- [ ] `valgrind` on each test binary → no leaks, exit 0, XML generated
- [ ] `gcovr --fail-under-line 100 --fail-under-branch 100 --json build/reports/gcovr.json` → exit 0 for each changed module

## Common Mistakes

| Mistake | Fix |
|---------|-----|
| No `FAKE_*_FUNC` for a called dependency | Declare fake and add `RESET_FAKE` in setUp |
| Forgetting `RESET_FAKE` in setUp | State leaks between tests; always reset in setUp, not tearDown |
| Checking `arg0_val` before checking `call_count` | Assert call_count first; arg values are undefined if not called |
| `DEFINE_FFF_GLOBALS` in a header | Move it to exactly one `.c` test file |
| Wrong macro for void function | Use `FAKE_VOID_FUNC`, not `FAKE_VALUE_FUNC` |
| Mixing CMock generated headers and FFF fakes | Pick one mocking approach per dependency |
| One test covering multiple branches | Split into one test per branch |
| Verifying too many internal details | Test behavior (return value, outputs), not implementation |
| Including implementation `.c` directly | Include only the public header |
