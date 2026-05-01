---
name: docx-review-comment-from-diff
description: Use when identifying which sections of an existing Word (.docx) specification document need updating based on a git diff. Triggered by requests to check a spec document against recent code changes, find docx sections affected by a diff, or produce a list of manual update instructions for a Word document.
---

# Specification Document Update Report from Git Diff

## Overview

Extract text from an existing `.docx` specification, search for symbols that changed in a `git diff`, and produce a Markdown report listing which sections need manual updating — with the current text excerpt and a concrete update instruction for each.

**The docx is not modified.** The report is for the human editor.

## Prerequisite Check

Before starting, verify a text extraction tool is available:

```bash
# Check python-docx
python3 -c "import docx; print('python-docx ok')" 2>/dev/null || echo "python-docx not found"

# Check pandoc
pandoc --version 2>/dev/null | head -1 || echo "pandoc not found"
```

If neither is available, stop and print:

```
ERROR: text extraction tool not found.
Install one before proceeding:
  pip install python-docx
  or
  sudo apt install pandoc   (Debian/Ubuntu)
  brew install pandoc       (macOS)
```

## Workflow

```text
git diff + docx path → extract docx text → collect changed symbols → search per symbol → output report
```

### Step 1: Extract docx Text with Heading Context

Use whichever tool is available:

```bash
# Option 1: python-docx (preserves heading context — preferred)
python3 - spec.docx << 'EOF'
import docx, sys
doc = docx.Document(sys.argv[1])
current_heading = "(preamble)"
for p in doc.paragraphs:
    if p.style.name.startswith("Heading"):
        current_heading = p.text.strip()
    elif p.text.strip():
        print(f"[{current_heading}] {p.text.strip()}")
EOF

# Option 2: pandoc (plain text, no heading context)
pandoc spec.docx -t plain -o /tmp/spec_plain.txt && cat /tmp/spec_plain.txt
```

### Step 2: Collect Changed Symbols from Diff

Run the diff and extract symbols:

```bash
git diff HEAD~1 HEAD   # adjust ref as needed
```

From the diff output, collect:

| Pattern | Extract |
| ------- | ------- |
| `^[+-].*\b([a-z_][a-z0-9_]+)\s*\(` | Function name |
| `^[+-]#define\s+([A-Z_][A-Z0-9_]+)` | Macro/constant name |
| `^[+-]typedef\s+.*\b(\w+)\s*;` | Type alias name |
| `^[+-]\s+([A-Z_][A-Z0-9_]+)\s*=` | Enum value name |

### Step 3: Search docx Text for Each Symbol

For each symbol, search the extracted text in this order:

1. **Exact symbol name** — e.g., `uart_send`
2. **Space-separated form** — e.g., `uart send`
3. **Related keywords** — for error codes: "エラーコード", "error code", "戻り値", "return value"; for types: "型定義", "typedef"

Record for each match:
- Heading context (from Step 1)
- Full paragraph text (first 200 characters)

### Step 4: Write the Report

## Output Format

```markdown
# Specification Update Report

**Date:** YYYY-MM-DD
**Source diff:** `git diff HEAD~1 HEAD`
**Target document:** spec.docx

---

## Sections Requiring Update

### 1. 第3章「UART送信関数の仕様」

- **Changed symbol:** `uart_send`
- **What changed:** Third parameter `timeout_ms` (type `uint32_t`) added; return code `UART_ERR_TIMEOUT` is new
- **Required change:** Add `timeout_ms` parameter to the argument table. Add `UART_ERR_TIMEOUT` to the return value table.
- **Current text (excerpt):**
  > "...`uart_send` は `buf` と `len` の2引数を受け取り、送信完了まで待機する..."

---

### 2. 第5章「エラーコード一覧」

- **Changed symbol:** `UART_ERR_TIMEOUT`
- **What changed:** New error code added
- **Required change:** Add a new row to the error code table: `UART_ERR_TIMEOUT | -2 | UART送信タイムアウト`
- **Current text (excerpt):**
  > "...エラーコード一覧を以下に示す..."

---

## Symbols Not Found in Document

The following changed symbols had no match in the document. New documentation sections may be needed:

| Symbol | Change type |
| ------ | ----------- |
| `uart_init_ex` | New function — no documentation exists yet |

---

## No Update Required

The following changed symbols were found in the document but the change does not affect their documented specification:

- `uart_flush` — internal implementation change only; public behavior unchanged
```

## Output File

Write to `docs/docx-review/YYYY-MM-DD-<docx-basename>-update-report.md` where `<docx-basename>` is the filename without extension:

```bash
mkdir -p docs/docx-review
```
