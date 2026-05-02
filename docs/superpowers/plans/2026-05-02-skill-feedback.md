# Skill Feedback Process Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a standard feedback collection step to the end of all 6 SKILL.md files so Claude automatically prompts for improvement feedback after each skill completes.

**Architecture:** Append identical Markdown text to the end of each SKILL.md. No new files, no new skills, no code — pure text edits. Each skill gets the same feedback step verbatim.

**Tech Stack:** Markdown, Claude Code skills

---

## File Structure

| File | Action |
| --- | --- |
| `skills/c-code-review-from-diff/SKILL.md` | Modify — append feedback step |
| `skills/ceedling-test-from-diff/SKILL.md` | Modify — append feedback step |
| `skills/change-spec-from-diff/SKILL.md` | Modify — append feedback step |
| `skills/docx-review-comment-from-diff/SKILL.md` | Modify — append feedback step |
| `skills/fff-mock-generation/SKILL.md` | Modify — append feedback step |
| `skills/technical-writing-review/SKILL.md` | Modify — append feedback step |

---

## The Feedback Step Text

This exact block is appended to every SKILL.md. Copy it precisely — do not paraphrase.

```markdown

---

### 最終ステップ: フィードバック収集

スキルの実行が完了しました。以下の点で問題はありましたか？

- **出力品質**: 見落とし・誤りがあった、期待と異なる結果になった
- **使い勝手**: 入力の渡し方が分かりにくい、出力フォーマットが使いにくい

**問題があった場合:** 具体的に説明してください。このSKILL.mdを改善してコミットします。  
**問題がなかった場合:** このステップはスキップしてください。
```

---

### Task 1: Add feedback step to c-code-review-from-diff and ceedling-test-from-diff

**Files:**
- Modify: `skills/c-code-review-from-diff/SKILL.md`
- Modify: `skills/ceedling-test-from-diff/SKILL.md`

- [ ] **Step 1: Append the feedback step to `skills/c-code-review-from-diff/SKILL.md`**

The current last line of this file is ` ``` ` (closing a code block). Append after it:

```
    /* handle timeout or error */
}
```

↓ file ends here. Add the following new content at the very end:

```markdown

---

### 最終ステップ: フィードバック収集

スキルの実行が完了しました。以下の点で問題はありましたか？

- **出力品質**: 見落とし・誤りがあった、期待と異なる結果になった
- **使い勝手**: 入力の渡し方が分かりにくい、出力フォーマットが使いにくい

**問題があった場合:** 具体的に説明してください。このSKILL.mdを改善してコミットします。  
**問題がなかった場合:** このステップはスキップしてください。
```

Use the Edit tool with `old_string` = the last 3 lines of the file:

```
    /* handle timeout or error */
}
```

And `new_string` = those same 3 lines plus the feedback block.

- [ ] **Step 2: Verify c-code-review-from-diff**

```bash
tail -15 /home/fsawa/skills/embedded_skills/skills/c-code-review-from-diff/SKILL.md
```

Expected: last section is `### 最終ステップ: フィードバック収集` with the two bullet points and the two bold lines.

- [ ] **Step 3: Append the feedback step to `skills/ceedling-test-from-diff/SKILL.md`**

The current last line is a table row. Append the feedback block using the Edit tool:

`old_string`:
```
| Including implementation `.c` directly | Include only the public header |
```

`new_string`:
```
| Including implementation `.c` directly | Include only the public header |

---

### 最終ステップ: フィードバック収集

スキルの実行が完了しました。以下の点で問題はありましたか？

- **出力品質**: 見落とし・誤りがあった、期待と異なる結果になった
- **使い勝手**: 入力の渡し方が分かりにくい、出力フォーマットが使いにくい

**問題があった場合:** 具体的に説明してください。このSKILL.mdを改善してコミットします。  
**問題がなかった場合:** このステップはスキップしてください。
```

- [ ] **Step 4: Verify ceedling-test-from-diff**

```bash
tail -15 /home/fsawa/skills/embedded_skills/skills/ceedling-test-from-diff/SKILL.md
```

Expected: last section is `### 最終ステップ: フィードバック収集`.

- [ ] **Step 5: Commit**

```bash
cd /home/fsawa/skills/embedded_skills
git add skills/c-code-review-from-diff/SKILL.md skills/ceedling-test-from-diff/SKILL.md
git commit -m "feat: add feedback collection step to c-code-review-from-diff and ceedling-test-from-diff"
```

---

### Task 2: Add feedback step to change-spec-from-diff and docx-review-comment-from-diff

**Files:**
- Modify: `skills/change-spec-from-diff/SKILL.md`
- Modify: `skills/docx-review-comment-from-diff/SKILL.md`

- [ ] **Step 1: Append the feedback step to `skills/change-spec-from-diff/SKILL.md`**

The current last 3 lines are:

```
```bash
mkdir -p docs/changes
```
```

`old_string`:
````
```bash
mkdir -p docs/changes
```
````

`new_string`:
````
```bash
mkdir -p docs/changes
```

---

### 最終ステップ: フィードバック収集

スキルの実行が完了しました。以下の点で問題はありましたか？

- **出力品質**: 見落とし・誤りがあった、期待と異なる結果になった
- **使い勝手**: 入力の渡し方が分かりにくい、出力フォーマットが使いにくい

**問題があった場合:** 具体的に説明してください。このSKILL.mdを改善してコミットします。  
**問題がなかった場合:** このステップはスキップしてください。
````

- [ ] **Step 2: Verify change-spec-from-diff**

```bash
tail -15 /home/fsawa/skills/embedded_skills/skills/change-spec-from-diff/SKILL.md
```

Expected: last section is `### 最終ステップ: フィードバック収集`.

- [ ] **Step 3: Append the feedback step to `skills/docx-review-comment-from-diff/SKILL.md`**

The current last 3 lines are:

```
```bash
mkdir -p docs/docx-review
```
```

`old_string`:
````
```bash
mkdir -p docs/docx-review
```
````

`new_string`:
````
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
````

- [ ] **Step 4: Verify docx-review-comment-from-diff**

```bash
tail -15 /home/fsawa/skills/embedded_skills/skills/docx-review-comment-from-diff/SKILL.md
```

Expected: last section is `### 最終ステップ: フィードバック収集`.

- [ ] **Step 5: Commit**

```bash
cd /home/fsawa/skills/embedded_skills
git add skills/change-spec-from-diff/SKILL.md skills/docx-review-comment-from-diff/SKILL.md
git commit -m "feat: add feedback collection step to change-spec-from-diff and docx-review-comment-from-diff"
```

---

### Task 3: Add feedback step to fff-mock-generation and technical-writing-review

**Files:**
- Modify: `skills/fff-mock-generation/SKILL.md`
- Modify: `skills/technical-writing-review/SKILL.md`

- [ ] **Step 1: Append the feedback step to `skills/fff-mock-generation/SKILL.md`**

The current last line is a table row. `old_string`:

```
| Using CMock `#include "mock_dep.h"` alongside FFF | Pick one. FFF needs no generated headers. |
```

`new_string`:

```
| Using CMock `#include "mock_dep.h"` alongside FFF | Pick one. FFF needs no generated headers. |

---

### 最終ステップ: フィードバック収集

スキルの実行が完了しました。以下の点で問題はありましたか？

- **出力品質**: 見落とし・誤りがあった、期待と異なる結果になった
- **使い勝手**: 入力の渡し方が分かりにくい、出力フォーマットが使いにくい

**問題があった場合:** 具体的に説明してください。このSKILL.mdを改善してコミットします。  
**問題がなかった場合:** このステップはスキップしてください。
```

- [ ] **Step 2: Verify fff-mock-generation**

```bash
tail -15 /home/fsawa/skills/embedded_skills/skills/fff-mock-generation/SKILL.md
```

Expected: last section is `### 最終ステップ: フィードバック収集`.

- [ ] **Step 3: Append the feedback step to `skills/technical-writing-review/SKILL.md`**

The current last line is `テキスト直接入力の場合は標準出力のみとし、ファイルには保存しない。`

`old_string`:

```
テキスト直接入力の場合は標準出力のみとし、ファイルには保存しない。
```

`new_string`:

```
テキスト直接入力の場合は標準出力のみとし、ファイルには保存しない。

---

### 最終ステップ: フィードバック収集

スキルの実行が完了しました。以下の点で問題はありましたか？

- **出力品質**: 見落とし・誤りがあった、期待と異なる結果になった
- **使い勝手**: 入力の渡し方が分かりにくい、出力フォーマットが使いにくい

**問題があった場合:** 具体的に説明してください。このSKILL.mdを改善してコミットします。  
**問題がなかった場合:** このステップはスキップしてください。
```

- [ ] **Step 4: Verify technical-writing-review**

```bash
tail -15 /home/fsawa/skills/embedded_skills/skills/technical-writing-review/SKILL.md
```

Expected: last section is `### 最終ステップ: フィードバック収集`.

- [ ] **Step 5: Commit**

```bash
cd /home/fsawa/skills/embedded_skills
git add skills/fff-mock-generation/SKILL.md skills/technical-writing-review/SKILL.md
git commit -m "feat: add feedback collection step to fff-mock-generation and technical-writing-review"
```

---

### Task 4: Reinstall all skills and push to GitHub

**Files:**
- `~/.claude/skills/` (installed skills directory)

- [ ] **Step 1: Reinstall all 6 skills to ~/.claude/skills/**

```bash
for skill in c-code-review-from-diff ceedling-test-from-diff change-spec-from-diff docx-review-comment-from-diff fff-mock-generation technical-writing-review; do
  mkdir -p ~/.claude/skills/$skill
  cp /home/fsawa/skills/embedded_skills/skills/$skill/SKILL.md ~/.claude/skills/$skill/SKILL.md
done
```

- [ ] **Step 2: Verify each installed skill has the feedback step**

```bash
for skill in c-code-review-from-diff ceedling-test-from-diff change-spec-from-diff docx-review-comment-from-diff fff-mock-generation technical-writing-review; do
  echo "=== $skill ==="
  grep -c "フィードバック収集" ~/.claude/skills/$skill/SKILL.md && echo "OK" || echo "MISSING"
done
```

Expected: each skill prints `1` and `OK`.

- [ ] **Step 3: Push to GitHub**

```bash
cd /home/fsawa/skills/embedded_skills
git remote -v
git push origin main
```

Expected: `main` pushed to `https://github.com/mimic-m/embedded_skills.git`.
