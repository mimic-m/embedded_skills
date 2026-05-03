# Skill Document Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Refactor the skill documents so names, links, operational wording, and known detection rules are consistent and less error-prone.

**Architecture:** This is a documentation-only refactor. Each `SKILL.md` remains self-contained because installed skills are consumed independently; common behavior is standardized by repeating concise, consistent wording where needed rather than introducing shared include files.

**Tech Stack:** Markdown skill documents, Git, shell verification with `rg`, `git diff --check`, and small POSIX shell loops.

---

## File Structure

- Modify: `README.md`  
  Keep the repository index accurate. Skill names, links, install command, workflow sequence, and tool requirements must match the actual `skills/*/SKILL.md` files.
- Modify: `skills/c-code-review-from-diff/SKILL.md`  
  Clarify ISR detection so strong ISR evidence and ambiguous handler naming are handled differently.
- Modify: `skills/static-analysis-report/SKILL.md`  
  Correct severity escalation rules, especially volatile handling and ISR evidence.
- Modify: `skills/docx-review-comment-from-diff/SKILL.md`  
  Count table rows during extraction validation so table-heavy `.docx` files are not rejected as scanned documents.
- Modify if repeated operational wording is inconsistent: `skills/ceedling-test-from-diff/SKILL.md`, `skills/change-spec-from-diff/SKILL.md`, `skills/extract-requirements-documents/SKILL.md`, `skills/fff-mock-generation/SKILL.md`, `skills/technical-writing-review/SKILL.md`  
  Normalize repeated operational wording only where drift is visible. Do not rewrite examples or change workflows without a concrete consistency issue.

---

### Task 1: README Name and Link Consistency

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Inspect actual skill directories**

Run:

```bash
find skills -mindepth 2 -maxdepth 2 -name SKILL.md | sort
```

Expected output includes exactly these paths:

```text
skills/c-code-review-from-diff/SKILL.md
skills/ceedling-test-from-diff/SKILL.md
skills/change-spec-from-diff/SKILL.md
skills/docx-review-comment-from-diff/SKILL.md
skills/extract-requirements-documents/SKILL.md
skills/fff-mock-generation/SKILL.md
skills/static-analysis-report/SKILL.md
skills/technical-writing-review/SKILL.md
```

- [ ] **Step 2: Update README skill references**

Edit `README.md` so every skill table row, section heading, installation-loop entry, and workflow entry uses the same canonical names from Step 1.

The install loop should contain all eight names:

```bash
for skill in extract-requirements-documents change-spec-from-diff static-analysis-report c-code-review-from-diff ceedling-test-from-diff fff-mock-generation docx-review-comment-from-diff technical-writing-review; do
  mkdir -p ~/.claude/skills/$skill
  cp skills/$skill/SKILL.md ~/.claude/skills/$skill/SKILL.md
done
```

- [ ] **Step 3: Verify README links exist**

Run:

```bash
for skill in extract-requirements-documents change-spec-from-diff static-analysis-report c-code-review-from-diff ceedling-test-from-diff fff-mock-generation docx-review-comment-from-diff technical-writing-review; do test -f "skills/$skill/SKILL.md" || exit 1; done
```

Expected: exit code `0`, no output.

- [ ] **Step 4: Commit README consistency changes**

Run:

```bash
git add README.md
git commit -m "docs: align README skill references"
```

Expected: commit succeeds if README changed. If no changes were needed, skip this commit and note that README was already consistent.

---

### Task 2: Correct Known Behavior Risks

**Files:**
- Modify: `skills/c-code-review-from-diff/SKILL.md`
- Modify: `skills/static-analysis-report/SKILL.md`
- Modify: `skills/docx-review-comment-from-diff/SKILL.md`

- [ ] **Step 1: Fix ISR detection wording in C code review skill**

In `skills/c-code-review-from-diff/SKILL.md`, replace the ISR-identification note with wording that separates strong and ambiguous signals:

```markdown
> **Identifying ISR handlers:** Treat a function as an ISR when there is strong evidence: the name matches `*_IRQHandler`, it is decorated with `__attribute__((interrupt))` or `__attribute__((isr))`, it appears in a vector table, or it is registered via `NVIC_SetVector` or an equivalent interrupt-vector API. A generic `*_Handler` name alone is ambiguous (for example `Error_Handler`); in that case, emit an `[INFO]` note asking the user to verify ISR context instead of automatically applying ISR-only severities.
```

- [ ] **Step 2: Fix ISR and volatile escalation in static analysis skill**

In `skills/static-analysis-report/SKILL.md`, update Step 3 so:

- ISR escalation uses the same strong-signal wording as Task 2 Step 1.
- Generic `*_Handler` names are ambiguous, not automatic escalation evidence.
- Volatile escalation requires evidence that a symbol is shared between ISR/main context or multiple RTOS tasks.
- Cppcheck IDs `unreadVariable` / `redundantAssignment` and MISRA Rule 8.6 / 19.x are not listed as volatile-missing evidence.

Use this replacement row for volatile handling:

```markdown
| volatile 漏れ | 解析結果または周辺コードから、同一シンボルが ISR とメイン処理、または複数 RTOS タスク間で共有されていると確認でき、かつ `volatile` または同期機構がない場合。静的解析結果だけで共有文脈を確認できない場合は昇格せず `[INFO]` として手動確認を促す |
```

- [ ] **Step 3: Fix docx extraction line counting**

In `skills/docx-review-comment-from-diff/SKILL.md`, replace the paragraph-only count snippet with a snippet that counts paragraphs and table rows:

```bash
# 抽出テキストの行数を確認する（段落 + 表行）
python3 - spec.docx << 'EOF'
import docx, sys
import docx.table
doc = docx.Document(sys.argv[1])
count = 0
for block in doc.element.body:
    tag = block.tag.split('}')[-1]
    if tag == 'p':
        text = ''.join(node.text or '' for node in block.iter() if node.tag.endswith('}t')).strip()
        if text:
            count += 1
    elif tag == 'tbl':
        tbl = docx.table.Table(block, doc)
        for row in tbl.rows:
            cells = [c.text.strip() for c in row.cells if c.text.strip()]
            if cells:
                count += 1
print(count)
EOF
```

- [ ] **Step 4: Verify no stale risky wording remains**

Run:

```bash
rg 'unreadVariable|redundantAssignment|MISRA Rule 8\\.6|19\\.x|\\*_Handler.*ISR|doc\\.paragraphs' skills/static-analysis-report/SKILL.md skills/c-code-review-from-diff/SKILL.md skills/docx-review-comment-from-diff/SKILL.md
```

Expected: no output for the removed volatile evidence and paragraph-only count. If `*_Handler` appears, it must be in text that describes ambiguity rather than automatic ISR classification.

- [ ] **Step 5: Commit behavior-risk fixes**

Run:

```bash
git add skills/c-code-review-from-diff/SKILL.md skills/static-analysis-report/SKILL.md skills/docx-review-comment-from-diff/SKILL.md
git commit -m "docs: correct skill detection guidance"
```

Expected: commit succeeds.

---

### Task 3: Normalize Repeated Operational Wording

**Files:**
- Modify if repeated operational wording is inconsistent: `skills/c-code-review-from-diff/SKILL.md`
- Modify if repeated operational wording is inconsistent: `skills/ceedling-test-from-diff/SKILL.md`
- Modify if repeated operational wording is inconsistent: `skills/change-spec-from-diff/SKILL.md`
- Modify if repeated operational wording is inconsistent: `skills/docx-review-comment-from-diff/SKILL.md`
- Modify if repeated operational wording is inconsistent: `skills/extract-requirements-documents/SKILL.md`
- Modify if repeated operational wording is inconsistent: `skills/fff-mock-generation/SKILL.md`
- Modify if repeated operational wording is inconsistent: `skills/static-analysis-report/SKILL.md`
- Modify if repeated operational wording is inconsistent: `skills/technical-writing-review/SKILL.md`

- [ ] **Step 1: Inspect repeated sections**

Run:

```bash
rg 'Output File|出力ファイル|フィードバック収集|Prerequisite Check|前提ツール確認|Inconclusive|INFO' skills/*/SKILL.md
```

Expected: output shows the repeated sections that need comparison.

- [ ] **Step 2: Normalize output-path wording**

For skills that write reports, use this pattern in their output-file section, translated only where the surrounding skill is already Japanese:

````markdown
Write to `docs/<category>/YYYY-MM-DD-<name>.md` relative to the repository root (the directory containing `.git/`). Create the directory before writing:

```bash
mkdir -p docs/<category>
```
````

Keep existing categories:

```text
c-code-review-from-diff        docs/reviews/
change-spec-from-diff          docs/changes/
docx-review-comment-from-diff  docs/docx-review/
extract-requirements-documents docs/requirements/
static-analysis-report         docs/static-analysis/
technical-writing-review       docs/reviews/
```

- [ ] **Step 3: Normalize feedback collection wording**

For skills that already have `### 最終ステップ: フィードバック収集`, keep the section but make the instruction consistent:

```markdown
### 最終ステップ: フィードバック収集

レポートまたは生成物を出力した後、ユーザーに以下を確認する：

- 指摘・生成内容に過不足がないか
- 次回から強めたい観点または弱めたい観点があるか
- プロジェクト固有ルールとして追加すべき判断基準があるか
```

Do not add this section to a skill that does not already have a feedback section unless another edited skill needs the same closeout to remain consistent.

- [ ] **Step 4: Keep skill behavior unchanged**

Review the diff manually:

```bash
git diff -- skills
```

Expected: changes are wording or correction changes only. There should be no new skill, no deleted skill, and no common include file.

- [ ] **Step 5: Commit operational wording cleanup**

Run:

```bash
git add skills
git commit -m "docs: normalize skill operational guidance"
```

Expected: commit succeeds if files changed. If no changes were needed beyond Task 2, skip this commit and note that no additional normalization was necessary.

---

### Task 4: Repository-Level Verification

**Files:**
- Verify: `README.md`
- Verify: `skills/*/SKILL.md`

- [ ] **Step 1: Check Git status**

Run:

```bash
git status --short --branch
```

Expected: on the working branch with either a clean tree or only intentional uncommitted verification edits. Before final completion, the tree must be clean.

- [ ] **Step 2: Check whitespace and conflict markers**

Run:

```bash
git diff --check HEAD
rg '<<<<<<<|=======|>>>>>>>' README.md skills
```

Expected:

- `git diff --check HEAD` exits `0`
- `rg` exits `1` with no output

- [ ] **Step 3: Verify frontmatter names match directory names**

Run:

```bash
for file in skills/*/SKILL.md; do dir="$(basename "$(dirname "$file")")"; name="$(sed -n 's/^name: //p' "$file" | head -1 | tr -d '"')"; test "$dir" = "$name" || { echo "$file: name=$name dir=$dir"; exit 1; }; done
```

Expected: exit code `0`, no output.

- [ ] **Step 4: Verify README listed skills exist**

Run:

```bash
for skill in extract-requirements-documents change-spec-from-diff static-analysis-report c-code-review-from-diff ceedling-test-from-diff fff-mock-generation docx-review-comment-from-diff technical-writing-review; do test -f "skills/$skill/SKILL.md" || { echo "missing $skill"; exit 1; }; done
```

Expected: exit code `0`, no output.

- [ ] **Step 5: Run markdown lint if available**

Run:

```bash
if command -v markdownlint >/dev/null 2>&1; then
  markdownlint README.md docs/**/*.md skills/*/SKILL.md
elif command -v markdownlint-cli2 >/dev/null 2>&1; then
  markdownlint-cli2 "README.md" "docs/**/*.md" "skills/*/SKILL.md"
else
  echo "markdownlint not available"
fi
```

Expected: lint exits `0`, or prints `markdownlint not available`. If unavailable, report that markdown linting was not run.

- [ ] **Step 6: Final clean status**

Run:

```bash
git status --short --branch
```

Expected: clean working tree.

---

## Self-Review Checklist

- [ ] Every requirement in `docs/superpowers/specs/2026-05-03-skill-doc-refactor-design.md` maps to a task above.
- [ ] No task introduces a common include file or runtime dependency.
- [ ] Verification includes whitespace, conflict markers, README links, frontmatter names, and markdown lint availability.
- [ ] Commits are small and grouped by responsibility.
