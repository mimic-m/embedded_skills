---
name: docx-review-comment-from-diff
description: Use when identifying which sections of an existing Word (.docx) specification document need updating based on a git diff. Triggered by requests to check a spec document against recent code changes, find docx sections affected by a diff, or produce a list of manual update instructions for a Word document.
---

# Specification Document Update Report from Git Diff

## Overview

Extract text from an existing `.docx` specification, search for symbols that changed in a `git diff`, and produce a Markdown report listing which sections need manual updating — with the current text excerpt and a concrete update instruction for each.

**The docx is not modified.** The report is for the human editor.

## How to Invoke

Provide:

1. **Path to the .docx file** — absolute or relative to the repository root, e.g., `docs/spec.docx`
2. **Git diff range** — optional, defaults to `HEAD~1 HEAD`. Examples: `HEAD~3 HEAD`, `origin/main...HEAD`

Example prompt:
> "Review `docs/hardware-spec.docx` against `git diff HEAD~1 HEAD`"

Run all commands from the repository root (the directory containing `.git/`).

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

If neither tool is confirmed available, halt. Do not proceed to Step 1.

## Workflow

```text
git diff + docx path → extract docx text → collect changed symbols → search per symbol → output report
```

### Step 1: Extract docx Text with Heading Context

Use whichever tool is available:

```bash
# Option 1: python-docx (preserves heading context and table rows — preferred)
python3 - spec.docx << 'EOF'
import docx, sys
import docx.text.paragraph, docx.table
doc = docx.Document(sys.argv[1])
current_heading = "(preamble)"
for block in doc.element.body:
    tag = block.tag.split('}')[-1]
    if tag == 'p':
        p = docx.text.paragraph.Paragraph(block, doc)
        if p.style.name.startswith("Heading"):
            current_heading = p.text.strip()
        elif p.text.strip():
            print(f"[{current_heading}] {p.text.strip()}")
    elif tag == 'tbl':
        tbl = docx.table.Table(block, doc)
        for row in tbl.rows:
            cells = [c.text.strip() for c in row.cells if c.text.strip()]
            if cells:
                print(f"[{current_heading}] TABLE_ROW: {' | '.join(cells)}")
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
3. **Related keywords by symbol type:**

| Symbol type | Japanese keywords | English keywords |
| ----------- | ----------------- | ---------------- |
| Function | 関数, 引数, 戻り値 | function, argument, return value |
| Macro/constant | マクロ, 定数 | macro, define, constant |
| Typedef | 型定義, 型 | typedef, type |
| Enum value | エラーコード, 列挙 | enum, error code, return value |

Record for each match:
- Heading context (from Step 1)
- Full paragraph text (first 200 characters)
- Whether it was found in a paragraph or a TABLE_ROW

After searching all symbols:
- Found with relevant diff context → add to "Sections Requiring Update"
- Found but change is internal/non-behavioral → add to "No Update Required"
- Not found in any search pass → add to "Symbols Not Found in Document"

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

Write to `docs/docx-review/YYYY-MM-DD-<docx-basename>-update-report.md` relative to the repository root (the directory containing `.git/`). `<docx-basename>` is the filename without extension:

```bash
mkdir -p docs/docx-review
```

---

### 最終ステップ: フィードバック収集

スキルの実行が完了しました。以下の点で問題はありましたか？

- **出力品質**: 見落とし・誤りがあった、期待と異なる結果になった
- **使い勝手**: 入力の渡し方が分かりにくい、出力フォーマットが使いにくい

**問題があった場合:** 具体的に説明してください。このSKILL.mdを改善してコミットします。  
**問題がなかった場合:** このステップはスキップしてください。
