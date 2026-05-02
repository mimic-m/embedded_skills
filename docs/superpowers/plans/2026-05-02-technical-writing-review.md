# Technical Writing Review Skill Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create the `technical-writing-review` Claude Code skill that reviews Japanese technical documents across 8 writing quality categories and outputs a severity-annotated Markdown report with before/after improvement suggestions.

**Architecture:** A single SKILL.md file under `skills/technical-writing-review/` following the same pattern as the other skills in this repo. No code — the skill is instructions for Claude. Install by copying to `~/.claude/skills/technical-writing-review/SKILL.md`. Verify manually by running the skill on a sample document.

**Tech Stack:** Markdown, Claude Code skills

---

## File Structure

| File | Action | Purpose |
|---|---|---|
| `skills/technical-writing-review/SKILL.md` | Create | The skill itself |
| `tests/sample-bad-japanese.md` | Create | Sample document with intentional writing issues for manual verification |
| `README.md` | Modify | Add the new skill to the skills table and usage section |

---

### Task 1: Create sample test document

**Files:**
- Create: `tests/sample-bad-japanese.md`

This document contains intentional issues covering all 8 categories. Used in Task 3 to verify the skill works correctly.

- [ ] **Step 1: Create the tests directory and sample document**

Create `tests/sample-bad-japanese.md` with this exact content:

```markdown
# テスト用サンプル文書

本文書では、HALドライバの初期化の実施について説明します。適切な設定を行うことで、
デバイスの動作が行われます。

## 初期化手順

初期化を行うには、以下の手順を実行する必要があります。

ファイルを開き、ヘッダを読み込み、設定値を解析し、レジスタへの書き込みを実行し、
完了フラグを設定する。

完了した場合、成功コードが返されますが、エラーが発生した場合はエラーコードが返されます
が、タイムアウトが発生した場合はタイムアウトコードが返されますが、その他のエラーについ
ては汎用エラーコード等が返される。

## 設定値

以下の設定値を使用することができます。

- タイムアウト値の設定（ミリ秒単位）
- リトライ回数を設定します
- ログレベルの指定

設定については以下を参照してください。また、注意事項については別紙を参照してください。
なお、詳細な設定方法については次章を参照してください。

## エラーコード

これを参照して処理を実装することができます。エラーが発生するという問題がある場合は、
スペック、仕様書、またはspecを確認してください。
```

- [ ] **Step 2: Commit the sample document**

```bash
git add tests/sample-bad-japanese.md
git commit -m "test: add sample Japanese document with intentional writing issues"
```

---

### Task 2: Create the SKILL.md

**Files:**
- Create: `skills/technical-writing-review/SKILL.md`

- [ ] **Step 1: Create the skill directory**

```bash
mkdir -p skills/technical-writing-review
```

- [ ] **Step 2: Create `skills/technical-writing-review/SKILL.md` with this exact content**

```markdown
---
name: technical-writing-review
description: Use when reviewing Japanese technical documents for writing quality. Applies 8 categories adapted from Google Technical Writing One: clear sentences, active voice, short sentences, consistent terminology, lists/tables, paragraph structure, document structure, and redundant expressions. Outputs a severity-annotated Markdown report with before/after improvement suggestions.
---

# 日本語技術文書レビュースキル

## 概要

日本語の技術文書（Markdown、テキストファイル）をGoogle Technical Writing Oneの原則を
日本語向けに適用した8カテゴリで分析し、指摘箇所と改善案をMarkdownレポートとして出力する。

## ワークフロー

```text
入力受付 → 8カテゴリでスキャン → 問題箇所の抽出 → before/after改善案の生成 → レポート出力
```

---

### Step 0: 入力を受け付ける

**ファイルパスが指定された場合:**

```bash
cat <指定されたパス>
```

**テキストが直接入力された場合:** そのテキストをレビュー対象とする。

**入力がない場合:** 「レビューする文書のパスまたはテキストを入力してください」と確認する。

---

### Step 1: 8カテゴリでスキャンする

以下の8カテゴリを順番に確認し、各カテゴリの問題をリストアップする。

#### カテゴリ1: 明確な文

以下のパターンを検索する:

| アンチパターン | 判定方法 | 深刻度 |
|---|---|---|
| 動詞の名詞化 | 「〜の実施」「〜の実行」「〜の設定」「〜の処理」で終わる名詞句が文中にある | MEDIUM |
| 曖昧な形容詞 | 「適切な」「必要な」「十分な」「良い」が具体的な数値や条件なしに使われている | MEDIUM |
| 「〜という」の多用 | 「〜というXがある」「〜ということ」のパターンが文中にある | LOW |

#### カテゴリ2: 能動態

以下のパターンを検索する:

| アンチパターン | 判定方法 | 深刻度 |
|---|---|---|
| 主語不明の受身 | 「〜が行われる」「〜が実行される」「〜が呼ばれる」「〜が返される」でアクターが不明 | HIGH |
| 複合受身 | 「〜されることになっている」「〜されるようになっている」 | MEDIUM |

Rule: 受身を見つけたら「誰が/何がその動作を行うか」を確認する。主語が特定できる場合は能動態に書き換える。

#### カテゴリ3: 短い文

以下のパターンを検索する:

| アンチパターン | 判定方法 | 深刻度 |
|---|---|---|
| 連用中止形の連続 | 「〜し、〜し、〜し」が3回以上連続している | MEDIUM |
| 逆説「が」の多用 | 1文に「〜するが、〜するが」が2回以上ある | HIGH |
| 4節以上の複文 | 「〜であり、〜であるため、〜が必要で、〜となる」のように読点で区切られた節が4つ以上 | HIGH |

#### カテゴリ4: 用語の一貫性

以下のパターンを検索する:

| アンチパターン | 判定方法 | 深刻度 |
|---|---|---|
| 同一概念の複数表記 | 同じ概念が「仕様書」「スペック」「spec」等で混在している | HIGH |
| 初出未定義の略語 | アルファベット大文字略語（HAL, ISR, API等）が初出時に日本語展開されていない | HIGH |
| 曖昧な代名詞 | 「これ」「それ」「当該」が指す名詞が直前の文から2文以上離れている | MEDIUM |

#### カテゴリ5: リストと表

以下のパターンを検索する:

| アンチパターン | 判定方法 | 深刻度 |
|---|---|---|
| 並列性の欠如 | 同一リスト内で体言止め（「〜の設定」）と動詞文末（「〜を設定します」）が混在 | MEDIUM |
| 番号付きリストの誤用 | 順序依存でない項目に番号付きリストを使用、または手順に箇条書きを使用 | MEDIUM |
| 1項目2行超え | 箇条書きの1項目が2行を超えている | LOW |

#### カテゴリ6: 段落構造

以下のパターンを検索する:

| アンチパターン | 判定方法 | 深刻度 |
|---|---|---|
| トピックセンテンスの欠如 | 段落の最初の文が段落全体のテーマを述べていない（例: 詳細から始まっている） | MEDIUM |
| 1段落複数トピック | 段落内で主語や話題が変わっている | MEDIUM |
| 長すぎる段落 | 1段落が6文以上 | LOW |

#### カテゴリ7: 文書構造

以下のパターンを検索する:

| アンチパターン | 判定方法 | 深刻度 |
|---|---|---|
| スコープ未定義 | 冒頭（最初の段落または見出し直下）にこの文書の目的の記述がない | HIGH |
| 見出しの不統一 | 見出しが体言止めと動詞文末（「〜する」「〜します」）で混在している | LOW |

#### カテゴリ8: 冗長表現

以下のパターンを検索する:

| アンチパターン | 検索パターン | 深刻度 |
|---|---|---|
| 〜することができる | 「することができ」 | LOW |
| 〜を行う/実施する | 「を行う」「を実施する」（直接動詞で言い換え可能なもの） | LOW |
| 〜等/〜など（曖昧） | 「等」「など」の直後に具体例や補足がない | LOW |
| 〜については | 「については」（「は」に短縮可能なもの） | LOW |
| 接続詞の連発 | 「なお、」「また、」「さらに、」が3回以上連続して文頭や段落頭に現れる | LOW |

---

### Step 2: 各問題に改善案を付ける

Step 1でリストアップした各問題に対して:

1. **箇所**: 問題のある文または句をそのまま引用する
2. **問題**: なぜ問題なのかを1文で説明する
3. **改善案**: 修正後の文または句を具体的に示す

改善案が複数ある場合は最も自然な1つを選ぶ。「要確認」のみの指摘は出力しない。

---

### Step 3: レポートを出力する

## 出力テンプレート

```markdown
# 技術文書レビュー: <ファイル名>

> **作成日:** YYYY-MM-DD
> **対象ファイル:** <パスまたは「直接入力」>

---

## サマリー

全N件の指摘（HIGH: X / MEDIUM: Y / LOW: Z）

---

## 指摘一覧

### [HIGH] 主語不明の受身（カテゴリ2: 能動態）

- **箇所:** 「設定が行われる。」
- **問題:** 誰が設定するのかが不明です。
- **改善案:** 「ドライバが設定する。」

### [MEDIUM] 動詞の名詞化（カテゴリ1: 明確な文）

- **箇所:** 「設定の実施後に再起動の実行が必要です。」
- **問題:** 動詞を名詞化すると文が重くなり読みにくくなります。
- **改善案:** 「設定後、再起動してください。」

### [LOW] 冗長表現（カテゴリ8: 冗長表現）

- **箇所:** 「設定することができます。」
- **問題:** 「することができる」は「できる」に短縮できます。
- **改善案:** 「設定できます。」

---

## 問題なし

以下のカテゴリは問題が検出されませんでした。

- カテゴリ3（短い文）: 文の長さは適切です。
- カテゴリ5（リストと表）: 箇条書きの並列性に問題はありません。
```

## 出力ファイル

ファイルパスが指定された場合は `docs/reviews/YYYY-MM-DD-<ファイル名>-writing-review.md` に保存する。

```bash
mkdir -p docs/reviews
```

テキスト直接入力の場合は標準出力のみとし、ファイルには保存しない。
```

- [ ] **Step 3: Commit the SKILL.md**

```bash
git add skills/technical-writing-review/SKILL.md
git commit -m "feat: add technical-writing-review skill for Japanese documents"
```

---

### Task 3: Verify the skill manually

**Files:**
- Read: `tests/sample-bad-japanese.md` (verification target)

This task verifies that the skill correctly detects all 8 categories of issues in the sample document.

- [ ] **Step 1: Install the skill to ~/.claude/skills/**

```bash
mkdir -p ~/.claude/skills/technical-writing-review
cp skills/technical-writing-review/SKILL.md ~/.claude/skills/technical-writing-review/SKILL.md
```

- [ ] **Step 2: Run the skill on the sample document**

In Claude Code, invoke:

```
/technical-writing-review tests/sample-bad-japanese.md
```

- [ ] **Step 3: Verify the output contains issues from all 8 categories**

Check the output report contains AT LEAST the following (one issue per category):

| Category | Expected issue in sample doc |
|---|---|
| カテゴリ1 明確な文 | 「適切な設定」（曖昧な形容詞） |
| カテゴリ2 能動態 | 「デバイスの動作が行われます」「成功コードが返されます」 |
| カテゴリ3 短い文 | 連用中止形（「ファイルを開き、〜し、〜し、〜し、〜する」） |
| カテゴリ4 用語の一貫性 | 「スペック」「仕様書」「spec」の混在、「HAL」未定義 |
| カテゴリ5 リストと表 | 「タイムアウト値の設定」（体言止め）と「リトライ回数を設定します」（文末）の混在 |
| カテゴリ6 段落構造 | （段落構造の問題がなければスキップ） |
| カテゴリ7 文書構造 | スコープ記述はある（冒頭に「〜について説明します」がある）→ 問題なし |
| カテゴリ8 冗長表現 | 「使用することができます」「を行う」「等」「については」「なお、」「また、」の連発 |

- [ ] **Step 4: If output is missing a category, update SKILL.md to fix the detection rule**

Re-check the detection pattern for the missing category in Step 1 of SKILL.md and tighten the description. Then re-run the skill and verify again.

---

### Task 4: Update README.md

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Read the current README.md**

```bash
cat README.md
```

- [ ] **Step 2: Add the new skill to the skills table**

In the `## スキル一覧` table, add this row:

```markdown
| [`technical-writing-review`](skills/technical-writing-review/SKILL.md) | 日本語技術文書をGoogle Technical Writing Oneの8カテゴリでレビューし、改善案付きレポートを出力する |
```

- [ ] **Step 3: Add the usage section**

After the `### docx-review-comment-from-diff` section, add:

```markdown
### `technical-writing-review`

```
technical-writing-reviewでREADME.mdをレビューしてください
```

日本語技術文書を8カテゴリで分析し、指摘箇所と改善案（before/after）を `docs/reviews/` 以下のMarkdownとして出力します。ファイルパスまたはテキストを直接入力として受け付けます。
```

- [ ] **Step 4: Add to the install script**

In the `## インストール` section, add `technical-writing-review` to the bash loop:

```bash
for skill in ceedling-test-from-diff fff-mock-generation c-code-review-from-diff change-spec docx-review-comment-from-diff technical-writing-review; do
  mkdir -p ~/.claude/skills/$skill
  cp skills/$skill/SKILL.md ~/.claude/skills/$skill/SKILL.md
done
```

- [ ] **Step 5: Commit README.md**

```bash
git add README.md
git commit -m "docs: add technical-writing-review skill to README"
```

---

### Task 5: Push to GitHub

- [ ] **Step 1: Check remote**

```bash
git remote -v
```

Expected: `origin  https://github.com/mimic-m/embedded_skills.git`

If remote is missing, add it:

```bash
git remote add origin https://github.com/mimic-m/embedded_skills.git
```

- [ ] **Step 2: Push**

```bash
git push origin main
```

- [ ] **Step 3: Verify on GitHub**

Confirm `skills/technical-writing-review/SKILL.md` is visible at `https://github.com/mimic-m/embedded_skills`.
