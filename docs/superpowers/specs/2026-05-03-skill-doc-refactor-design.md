# Skill Document Refactor Design

**Date:** 2026-05-03
**Scope:** Refactor README and all `skills/*/SKILL.md` files for consistency and correctness

---

## Purpose

This refactor improves the embedded skills repository as a set of executable skill documents. The primary goal is to reduce incorrect agent behavior caused by inconsistent names, overly broad detection rules, and duplicated instructions that have drifted across skills.

The refactor is intentionally documentation-only. It does not add new skills, remove existing skills, or introduce generated helper files that a skill would need to load separately.

---

## Requirements

### R1: Keep Skill Names and Links Consistent

All skill names in `README.md`, installation commands, workflow examples, and frontmatter must match the actual directories under `skills/`.

The canonical skill names are:

- `extract-requirements-documents`
- `change-spec-from-diff`
- `static-analysis-report`
- `c-code-review-from-diff`
- `ceedling-test-from-diff`
- `fff-mock-generation`
- `docx-review-comment-from-diff`
- `technical-writing-review`

### R2: Fix Known Incorrect Instructions

The refactor must fix three known behavior risks:

- `static-analysis-report` must not treat unrelated Cppcheck IDs or MISRA 8.6 / 19.x findings as direct evidence of missing `volatile`.
- ISR detection must avoid treating every `*_Handler` function as an ISR. `*_IRQHandler`, interrupt attributes, vector-table registration, and equivalent explicit registration are strong ISR signals. Generic `*_Handler` names are ambiguous unless supported by surrounding context.
- `docx-review-comment-from-diff` must count table rows when deciding whether a `.docx` extraction produced enough text. A table-heavy specification must not be mislabeled as image-only solely because it has few paragraphs.

### R3: Standardize Repeated Operational Sections

Repeated instructions should use consistent wording where possible:

- prerequisite checks
- output directory creation
- output file paths
- feedback collection
- inconclusive or ambiguous cases

Each `SKILL.md` must remain self-contained. Do not move shared text into a common include file because installed skills are read independently.

### R4: Preserve Existing Skill Behavior

The refactor should not change the intended user-facing workflow of each skill except where the current instructions are inaccurate. Examples and templates may be clarified, but the output locations and main workflow steps should remain recognizable.

### R5: Verification

Before completion, run local checks that prove the repository is structurally coherent:

- `git diff --check`
- search for conflict markers in README and skill files
- verify every README skill link points to an existing `SKILL.md`
- verify every `skills/*/SKILL.md` frontmatter `name:` matches its directory name
- run markdown linting if an available linter exists in the environment

If markdown linting cannot be run because no linter is installed and no project command exists, report that explicitly.

---

## Proposed Changes

### README

Keep README as the index and workflow entry point. It should list every skill once, group them by workflow stage, and use only real skill paths.

### C/C++ Review Skill

Clarify ISR detection so strong signals are separated from ambiguous handler names. Ambiguous handler names should produce an INFO note or a manual-verification note instead of automatic CRITICAL/HIGH findings.

### Static Analysis Report Skill

Refine severity escalation rules:

- keep heap-operation, ISR-context, large-stack, and uninitialized-variable escalation where evidence exists
- replace the volatile escalation rule with a conservative rule requiring shared-context evidence
- note when volatile correctness cannot be inferred from static-analysis output alone

### Docx Review Skill

Use one extraction model for both extraction and text-count validation: count non-empty paragraphs and non-empty table rows. Keep the OCR warning only for documents with genuinely low extracted text.

### Shared Consistency

Review all skill files for drift in output-path wording and final feedback collection language. Normalize wording without introducing external dependencies.

---

## Non-Goals

- No new skill is added.
- No existing skill is removed.
- No common shared include file is introduced.
- No runtime tool is added to the repository.
- No broad rewrite of all examples is required unless an example contradicts the corrected behavior.

---

## Risks

The main risk is over-normalizing skill text and making individual skills less useful in isolation. To avoid that, each skill keeps its own workflow, examples, and output template.

Another risk is making detection rules too weak. The corrected ISR and volatile rules should prefer explicit evidence and add INFO notes for ambiguous cases rather than silently ignoring them.

---

## Acceptance Criteria

- README links and installation command match the actual skill directories.
- Known incorrect rules for volatile escalation, ISR detection, and `.docx` line counting are corrected.
- Skill files remain self-contained.
- Verification commands complete with no conflict markers or whitespace errors.
- Any unavailable verification tool is reported clearly.
