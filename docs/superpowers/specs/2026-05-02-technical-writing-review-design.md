# Technical Writing Review Skill Design

**Date:** 2026-05-02
**Scope:** 1 new Claude Code skill for reviewing Japanese technical documents

---

## Overview

A skill that reviews Japanese technical documents against 8 writing quality categories adapted from Google Technical Writing One. For each issue found, it provides a concrete before/after rewrite suggestion.

**Target documents:** Markdown files, plain text files (README, SKILL.md, spec documents, etc.)

**Target language:** Japanese

---

## Skill: `technical-writing-review`

### Purpose

Review a Japanese technical document and output a structured Markdown report that lists issues by category, severity, and concrete improvement suggestions.

### Trigger

User provides a file path or pasted text and requests a writing quality review.

### Workflow

```text
入力受付 → 8カテゴリでスキャン → 問題箇所の抽出 → before/after改善案の生成 → レポート出力
```

---

## Check Categories

### Category 1: 明確な文 (Clear Sentences)

Check for patterns that reduce sentence clarity:

| Anti-pattern | Example | Fix |
|---|---|---|
| 動詞の名詞化 | 「設定の実施後に再起動の実行が必要です。」 | 「設定後、再起動してください。」 |
| 曖昧な形容詞 | 「適切な値を設定する」 | 「10〜100 の整数値を設定する」 |
| 「〜という」の多用 | 「エラーが発生するという問題がある」 | 「エラーが発生する問題がある」 |

### Category 2: 能動態 (Active Voice)

Check for excessive passive constructions:

| Anti-pattern | Example | Fix |
|---|---|---|
| 主語不明の受身 | 「設定が行われる」 | 「ドライバが設定する」 |
| 複合受身 | 「処理が実行されることになっている」 | 「処理を実行する」 |

Rule: `主語 + 動詞 + 目的語` の順で書けるか確認する。できない場合は受身を疑う。

### Category 3: 短い文 (Short Sentences)

Check for overly long sentences:

| Anti-pattern | Example | Fix |
|---|---|---|
| 連用中止形の連続 | 「ファイルを開き、データを読み込み、変換し、保存する」 | 番号付きリストに分割 |
| 逆説「が」の多用 | 「処理は完了するが、結果は保存されるが、ログは出力されない」 | 文を分割 |
| 3節以上の複文 | 「〜であり、〜であるため、〜が必要で、〜となる」 | 2文以内に分割 |

### Category 4: 用語の一貫性 (Consistent Terminology)

Check for terminology issues:

| Anti-pattern | Example | Fix |
|---|---|---|
| 同一概念の複数表記 | 「仕様書」「スペック」「spec」の混在 | 1つに統一し、初出で定義 |
| 初出未定義の略語 | 「HALを使用する」 | 「Hardware Abstraction Layer（HAL）を使用する」 |
| 曖昧な代名詞 | 「これを呼び出す」（何が？） | 具体名詞に置換 |

### Category 5: リストと表 (Lists and Tables)

Check for list and table formatting issues:

| Issue | Check |
|---|---|
| 並列性の欠如 | 箇条書き内で体言止めと文末混在していないか |
| 番号付きリストの誤用 | 順序不要な手順に番号を使っていないか（逆も然り） |
| 表の重複 | 表ヘッダと内容が同じ情報を繰り返していないか |
| 長すぎる箇条書き | 1項目が2行を超えていないか |

### Category 6: 段落構造 (Paragraph Structure)

Check for paragraph issues:

| Issue | Check |
|---|---|
| トピックセンテンスの欠如 | 段落の最初の文が段落全体のテーマを述べているか |
| 1段落複数トピック | 段落内で話題が変わっていないか |
| 長すぎる段落 | 5文を超えていないか |

### Category 7: 文書構造 (Document Structure)

Check for document-level issues:

| Issue | Check |
|---|---|
| スコープ未定義 | 文書の冒頭でこの文書が何を説明するか述べているか |
| 対象読者未定義 | 誰を読者として想定しているか明示されているか |
| 見出しの一貫性 | 見出しが体言止めか文末かで統一されているか |

### Category 8: 冗長表現 (Redundant Expressions)

Check for verbose Japanese patterns:

| Anti-pattern | Example | Fix |
|---|---|---|
| 〜することができる | 「設定することができます」 | 「設定できます」 |
| 〜を行う/実施する | 「初期化を行う」 | 「初期化する」 |
| 〜等/〜など（曖昧） | 「エラーコード等を返す」 | 「エラーコードを返す」（または具体列挙） |
| 〜については | 「設定値については以下を参照」 | 「設定値は以下を参照」 |
| 「なお、」「また、」の連発 | 段落冒頭に3回以上 | 削除または段落を統合 |

---

## Severity Levels

| Level | Meaning | Action |
|---|---|---|
| HIGH | 意味の誤解や読み違いを招く | 修正を推奨 |
| MEDIUM | 読みにくさを生む | 修正を検討 |
| LOW | スタイルの改善 | 任意 |

CRITICAL は使用しない（安全性に関わらないため）。

---

## Output Format

```markdown
# 技術文書レビュー: <ファイル名>

> **作成日:** YYYY-MM-DD
> **対象ファイル:** <パス>

---

## サマリー

全N件の指摘（HIGH: X / MEDIUM: Y / LOW: Z）

---

## 指摘一覧

### [HIGH] 動詞の名詞化（カテゴリ1: 明確な文）

- **箇所:** 「設定の実施後に再起動の実行が必要です。」
- **問題:** 動詞を名詞化すると文が重く読みにくくなります。
- **改善案:** 「設定後、再起動してください。」

...

---

## 問題なし

以下のカテゴリは問題が検出されませんでした。

- カテゴリ4（用語の一貫性）: 全用語が一貫して使用されています。
- カテゴリ5（リストと表）: 箇条書きの並列性に問題はありません。
```

---

## Output File

`docs/reviews/YYYY-MM-DD-<filename>-writing-review.md`（リポジトリルートからの相対パス）

ファイルパス指定がない場合（テキスト直接入力）は標準出力のみとする。

---

## Common Conventions

- 入力ファイルがない場合は「レビューする文書のパスまたはテキストを入力してください」と確認する
- 指摘が0件の場合は「指摘なし」と明記し、各カテゴリの確認済み旨を記載する
- 改善案は必ず1つ以上提示する（「要検討」のみの指摘は出力しない）
