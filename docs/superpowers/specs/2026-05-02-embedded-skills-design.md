# Embedded Skills Design

**Date:** 2026-05-02
**Scope:** 3 new Claude Code skills for embedded C/C++ development workflows

---

## Overview

Three independent skills that each accept `git diff` as primary input and produce structured Markdown output. All three follow the workflow-driven pattern established by `ceedling-test-from-diff` and `fff-mock-generation`.

---

## Skill 1: `c-code-review-from-diff`

### Skill 1 Purpose

Review C/C++ embedded code changes from `git diff` and produce a severity-annotated Markdown report.

### Skill 1 Trigger

User requests a code review of recent changes in an embedded C/C++ project.

### Skill 1 Workflow

```text
git diff → parse diff → extract changed functions → embedded checks → quality checks → output report
```

### Review Layers

**Embedded-specific:**

- Memory management: no `malloc`/`free` in ISR, stack depth, static allocation patterns
- Interrupt safety: `volatile` on shared variables, critical section protection, no blocking calls in ISR
- HAL boundary violations: direct register access outside HAL layer
- Timeout handling: blocking I/O must have timeout
- Magic register addresses: bare hex addresses without named constants

**General quality:**

- Function size (< 50 lines)
- Naming conventions
- Nesting depth (max 4 levels, prefer early return)
- Error handling completeness: every return value checked
- Magic numbers: use named constants
- Testability: dependencies injectable, no hidden globals

### Severity Levels

| Level    | Meaning                                      | Action                    |
| -------- | -------------------------------------------- | ------------------------- |
| CRITICAL | Safety risk, data corruption, undefined behavior | Must fix before merge |
| HIGH     | Bug or significant quality issue             | Should fix before merge   |
| MEDIUM   | Maintainability concern                      | Consider fixing           |
| LOW      | Style or minor suggestion                    | Optional                  |

### Skill 1 Output Format

```markdown
# Code Review Report
## Summary
<1-3 sentence overview of the change>

## Issues

### [CRITICAL] <title>
- **File:** src/uart.c:42
- **Reason:** <explanation>
- **Fix:** <concrete suggestion>

### [HIGH] <title>
...

## No Issues Found
<list of checked items that passed>
```

### Skill 1 Output File

Written to `docs/reviews/YYYY-MM-DD-<module>-review.md` (optional; can also print to stdout).

---

## Skill 2: `change-spec-from-diff`

### Skill 2 Purpose

Generate a human-readable change specification document in Markdown from `git diff` output.

### Skill 2 Trigger

User needs to document what changed for handoff, release notes, or traceability.

### Skill 2 Workflow

```text
git diff → classify changes by module/function → determine change type → expand into template → write Markdown
```

### Change Type Classification

| Diff pattern                              | Change type      |
| ----------------------------------------- | ---------------- |
| New function added                        | New feature      |
| Function signature changed                | Interface change |
| Function body modified (signature unchanged) | Behavior change |
| Function deleted                          | Feature removal  |
| Macro/constant changed                    | Constant change  |
| File added                                | New module       |
| File deleted                              | Module removal   |

### Skill 2 Output Template

```markdown
# Change Specification
**Date:** YYYY-MM-DD
**Branch / Commit:** <ref>
**Author:** <from git log>

## Summary
<1-3 sentence plain-language summary of what changed and why>

## Changed Modules

### <module_name>

| Item | Before | After |
|---|---|---|
| <function/constant> | <old behavior/value> | <new behavior/value> |

**Change type:** Interface change / Behavior change / New feature / ...
**Impact:** <which callers or tests are affected>

## Compatibility Notes
<breaking changes, migration steps, or "No breaking changes">

## Related Tests
<list of test files that cover the changed functions, or "None — tests needed">
```

### Skill 2 Output File

`docs/changes/YYYY-MM-DD-<module>-change-spec.md`

---

## Skill 3: `docx-review-comment-from-diff`

### Skill 3 Purpose

Given a `git diff` and an existing `.docx` specification document, produce a Markdown report listing exactly which sections of the docx need to be updated and what to change.

### Skill 3 Trigger

User has an existing Word specification document and wants to know which parts need to be updated to reflect recent code changes.

### Skill 3 Workflow

```text
git diff + docx → extract docx text → search for affected symbols → identify impacted sections → output comment report
```

### docx Text Extraction

The skill checks for available tools in this order:

```bash
# Option 1: python-docx (preferred)
python3 -c "import docx; [print(p.text) for p in docx.Document('spec.docx').paragraphs]"

# Option 2: pandoc fallback
pandoc spec.docx -t plain -o spec.txt

# Option 3: warn and exit
echo "Neither python-docx nor pandoc found. Install one to continue."
```

### Symbol Search Strategy

For each changed function/constant/type from the diff:

1. Search docx text for exact symbol name
2. Search for common aliases (e.g., `uart_send` → "UART送信", "uart send")
3. Search for related section headings (e.g., "エラーコード", "戻り値")
4. Record paragraph text and approximate location (heading path)

### Skill 3 Output Format

```markdown
# Specification Update Report
**Date:** YYYY-MM-DD
**Source diff:** <git ref>
**Target document:** <filename>

## Sections Requiring Update

### 1. <Chapter/Section heading>
- **Reason:** <which symbol changed and how>
- **Required change:** <concrete instruction for the human editor>
- **Current text (excerpt):** "...exact text from the docx paragraph..."

### 2. <Chapter/Section heading>
...

## Symbols Not Found in Document
<list of changed symbols with no match in the docx — may need new sections>

## No Update Required
<list of changed symbols confirmed to have no mention in the docx>
```

### Skill 3 Output File

`docs/docx-review/YYYY-MM-DD-<docx-basename>-update-report.md`

---

## Common Conventions (all three skills)

- Input: `git diff HEAD`, `git diff HEAD~1 HEAD`, or `git diff <base>..<head>` — user specifies
- All skills are invoked independently; no pipeline between them
- Target language: C/C++ embedded (skills are not language-agnostic)
- Output files committed to `docs/` under the repo; user may override path
- Severity definitions shared with `ceedling-test-from-diff`: CRITICAL / HIGH / MEDIUM / LOW
