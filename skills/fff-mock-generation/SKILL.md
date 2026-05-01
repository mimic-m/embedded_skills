---
name: fff-mock-generation
description: Use when creating or updating C mock/fake functions with FFF (Fake Function Framework) from git diff output. Triggered by requests to mock C dependencies using fff.h, generate FAKE_VOID_FUNC/FAKE_VALUE_FUNC declarations, or replace CMock with FFF in Ceedling tests.
---

# FFF Mock Generation from Git Diff

## Overview

FFF (Fake Function Framework) generates fakes via macros in a single header (`fff.h`). Unlike CMock, fakes are declared manually — no code generation tool required.

**Used with:** `ceedling-test-from-diff` skill. That skill covers test structure, Unity assertions, and verification. This skill covers only the FFF-specific mock layer.

## Workflow

```
git diff → identify external function calls → declare FAKE_*_FUNC → configure in setUp → assert in tests
```

### Step 1: Identify What to Mock

From the diff, find every function call that crosses module boundaries (i.e., defined in another `.c` file):

```diff
+    status = uart_send(buf, len);      → mock uart_send
+    ts = rtc_get_timestamp();          → mock rtc_get_timestamp
+    led_set(LED_RED, true);            → mock led_set (void)
```

### Step 2: Declare Fakes

Place fake declarations at the **top of the test file**, after `#include "fff.h"`:

```c
#include "unity.h"
#include "fff.h"
#include "<module_under_test>.h"

DEFINE_FFF_GLOBALS;

/* void function: FAKE_VOID_FUNC(name, arg_type, ...) */
FAKE_VOID_FUNC(led_set, led_id_t, bool);

/* function returning a value: FAKE_VALUE_FUNC(return_type, name, arg_type, ...) */
FAKE_VALUE_FUNC(int,      uart_send,       const uint8_t*, size_t);
FAKE_VALUE_FUNC(uint32_t, rtc_get_timestamp);
```

One `DEFINE_FFF_GLOBALS;` per translation unit (test file). Never in a header.

## FFF Macro Reference

| Signature | Macro |
|-----------|-------|
| `void f(void)` | `FAKE_VOID_FUNC(f)` |
| `void f(int, char*)` | `FAKE_VOID_FUNC(f, int, char*)` |
| `int f(void)` | `FAKE_VALUE_FUNC(int, f)` |
| `int f(int, int)` | `FAKE_VALUE_FUNC(int, f, int, int)` |
| `void f(int, ...)` | `FAKE_VOID_FUNC_VARARG(f, int, ...)` |
| `int f(int, ...)` | `FAKE_VALUE_FUNC_VARARG(int, f, int, ...)` |

Maximum default argument count is 20. Override with `#define FFF_ARG_HISTORY_LEN N` before including `fff.h`.

## setUp and tearDown

```c
void setUp(void) {
    RESET_FAKE(uart_send);
    RESET_FAKE(rtc_get_timestamp);
    RESET_FAKE(led_set);
    FFF_RESET_HISTORY();
}

void tearDown(void) { }
```

Always call `RESET_FAKE` for every fake in `setUp` — not `tearDown` — so state is clean before each test.

## Configuring Fake Behavior

### Set return value

```c
uart_send_fake.return_val = UART_OK;
```

### Return a sequence of values (different per call)

```c
int seq[] = { UART_OK, UART_ERR_BUSY, UART_OK };
SET_RETURN_SEQ(uart_send, seq, 3);
```

### Custom callback (full control)

```c
static int my_uart_send(const uint8_t *buf, size_t len) {
    TEST_ASSERT_EQUAL_HEX8(0xAA, buf[0]);
    return UART_OK;
}
uart_send_fake.custom_fake = my_uart_send;
```

## Asserting on Fake State

```c
/* call count */
TEST_ASSERT_EQUAL_INT(1, uart_send_fake.call_count);
TEST_ASSERT_EQUAL_INT(0, led_set_fake.call_count);   /* not called */

/* argument values of the most recent call */
TEST_ASSERT_EQUAL_PTR(expected_buf, uart_send_fake.arg0_val);
TEST_ASSERT_EQUAL_INT(expected_len, uart_send_fake.arg1_val);

/* argument from a specific call (0-indexed) */
TEST_ASSERT_EQUAL_INT(3, uart_send_fake.arg1_history[2]);  /* 3rd call, 2nd arg */
```

Argument fields: `arg0_val`, `arg1_val`, … `argN_val` (most recent call).  
History arrays: `arg0_history[i]`, `arg1_history[i]`, … (up to `FFF_ARG_HISTORY_LEN` calls).

## Complete Test File Example

**Diff shows:**
```diff
+int packet_send(packet_t *pkt) {
+    if (pkt == NULL) return PKT_ERR_NULL;
+    uint32_t ts = rtc_get_timestamp();
+    pkt->timestamp = ts;
+    return uart_send(pkt->data, pkt->len);
+}
```

**Generated test file:**
```c
#include "unity.h"
#include "fff.h"
#include "packet.h"

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(uint32_t, rtc_get_timestamp);
FAKE_VALUE_FUNC(int,      uart_send, const uint8_t*, size_t);

void setUp(void) {
    RESET_FAKE(rtc_get_timestamp);
    RESET_FAKE(uart_send);
    FFF_RESET_HISTORY();
}

void tearDown(void) { }

void test_packet_send_returns_error_on_null(void) {
    TEST_ASSERT_EQUAL_INT(PKT_ERR_NULL, packet_send(NULL));
    TEST_ASSERT_EQUAL_INT(0, uart_send_fake.call_count);
}

void test_packet_send_stamps_timestamp_and_sends(void) {
    uint8_t data[] = { 0xDE, 0xAD };
    packet_t pkt = { .data = data, .len = 2 };
    rtc_get_timestamp_fake.return_val = 0xCAFEBABE;
    uart_send_fake.return_val = UART_OK;

    int result = packet_send(&pkt);

    TEST_ASSERT_EQUAL_INT(PKT_OK,     result);
    TEST_ASSERT_EQUAL_HEX32(0xCAFEBABE, pkt.timestamp);
    TEST_ASSERT_EQUAL_INT(1, uart_send_fake.call_count);
    TEST_ASSERT_EQUAL_PTR(data,       uart_send_fake.arg0_val);
    TEST_ASSERT_EQUAL_INT(2,          uart_send_fake.arg1_val);
}
```

## Ceedling project.yml Setup

FFF requires no Ceedling plugin — just add `fff.h` to the include path:

```yaml
:paths:
  :include:
    - test/support     # place fff.h here

:defines:
  :test:
    - FFF_ARG_HISTORY_LEN=20   # optional: increase if functions have many calls
```

Download `fff.h` from the [meekrosoft/fff](https://github.com/meekrosoft/fff) repository (single-header, no build step).

## Diff → Fake Declaration Decision Table

| Diff pattern | Macro to use |
|---|---|
| New call to `void dep(void)` | `FAKE_VOID_FUNC(dep)` |
| New call to `void dep(T1, T2)` | `FAKE_VOID_FUNC(dep, T1, T2)` |
| New call to `T dep(void)` | `FAKE_VALUE_FUNC(T, dep)` |
| New call to `T dep(T1, T2)` | `FAKE_VALUE_FUNC(T, dep, T1, T2)` |
| Removed call to previously mocked dep | Remove `FAKE_*` declaration and all related assertions |
| Changed signature of mocked dep | Update `FAKE_*` macro arguments to match new signature |

## Common Mistakes

| Mistake | Fix |
|---------|-----|
| `DEFINE_FFF_GLOBALS` in header | Move to exactly one `.c` test file |
| Resetting in `tearDown` not `setUp` | Reset in `setUp` — next test starts clean |
| `arg0_val` without checking `call_count` | Always assert `call_count` first |
| Forgetting `FFF_RESET_HISTORY()` | Global call order history persists across tests without it |
| Wrong macro for void function | `void` return → `FAKE_VOID_FUNC`, not `FAKE_VALUE_FUNC` |
| Using CMock `#include "mock_dep.h"` alongside FFF | Pick one. FFF needs no generated headers. |
