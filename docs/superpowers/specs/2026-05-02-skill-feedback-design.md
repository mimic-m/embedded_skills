# Skill Feedback Process Design

**Date:** 2026-05-02
**Scope:** Add a feedback collection step to all 6 existing skills

---

## Overview

When a developer uses a skill in daily work and the task completes, Claude automatically asks whether there were any problems with the skill's output quality or usability. If the developer reports an issue, Claude immediately improves the SKILL.md and commits the change.

**Target language:** Japanese (all user-facing text in feedback step is Japanese)

---

## Problem

Skills can degrade silently: detection rules miss new patterns, output formats prove awkward in practice, or input conventions are unclear. Currently there is no mechanism to capture this feedback and act on it.

---

## Design

### Feedback Step (standard text added to every SKILL.md)

Add the following as the final step of each skill, after all output has been produced:

```markdown
### 最終ステップ: フィードバック収集

スキルの実行が完了しました。以下の点で問題はありましたか？

- **出力品質**: 見落とし・誤りがあった、期待と異なる結果になった
- **使い勝手**: 入力の渡し方が分かりにくい、出力フォーマットが使いにくい

**問題があった場合:** 具体的に説明してください。このSKILL.mdを改善してコミットします。  
**問題がなかった場合:** このステップはスキップしてください。
```

### Flow

```text
スキル完了
  ↓
フィードバック収集ステップを実行
  ├─ 問題なし → 終了（スキップ）
  └─ 問題あり → ユーザーが説明
                  ↓
               SKILL.md の該当箇所を特定して修正案を提示
                  ↓
               ユーザーが承認
                  ↓
               SKILL.md を編集してコミット
               (fix: improve <skill-name> based on usage feedback)
```

### Improvement Commit Convention

```text
fix: improve <skill-name> based on usage feedback

<問題の概要>
<修正内容の概要>
```

---

## Target Skills

| スキル | SKILL.mdパス |
| --- | --- |
| `change-spec` | `skills/change-spec/SKILL.md` |
| `c-code-review-from-diff` | `skills/c-code-review-from-diff/SKILL.md` |
| `ceedling-test-from-diff` | `skills/ceedling-test-from-diff/SKILL.md` |
| `fff-mock-generation` | `skills/fff-mock-generation/SKILL.md` |
| `docx-review-comment-from-diff` | `skills/docx-review-comment-from-diff/SKILL.md` |
| `technical-writing-review` | `skills/technical-writing-review/SKILL.md` |

---

## Constraints

- The feedback step must be short and non-intrusive. Developers who have no issues can skip it with a single word.
- Improvements are applied immediately to the SKILL.md in the repo and installed to `~/.claude/skills/` in the same commit.
- No new skill files are created. This is a text addition to existing SKILL.md files only.
