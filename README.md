# embedded_skills

組み込み C/C++ 開発向けの Claude Code スキル集です。  
`git diff` を入力として、テスト生成・コードレビュー・変更仕様書作成・仕様書更新箇所の特定を自動化します。

---

## スキル一覧

| スキル | 概要 |
| ------ | ---- |
| [`ceedling-test-from-diff`](skills/ceedling-test-from-diff/SKILL.md) | git diff から Ceedling ユニットテスト（`test_*.c`）を生成・更新する |
| [`fff-mock-generation`](skills/fff-mock-generation/SKILL.md) | git diff から FFF フェイク関数宣言（`FAKE_VOID_FUNC` / `FAKE_VALUE_FUNC`）を生成する |
| [`c-code-review-from-diff`](skills/c-code-review-from-diff/SKILL.md) | git diff の変更箇所を組み込み観点＋一般品質観点でレビューし、重大度付きレポートを出力する |
| [`change-spec-from-diff`](skills/change-spec-from-diff/SKILL.md) | git diff から変更仕様書（Markdown）を生成する |
| [`docx-review-comment-from-diff`](skills/docx-review-comment-from-diff/SKILL.md) | git diff をもとに既存 Word 仕様書（.docx）の修正が必要な箇所を特定し、レポートを出力する |

---

## インストール

各スキルの `SKILL.md` を Claude Code のスキルディレクトリにコピーします。

```bash
# 例: すべてのスキルをインストール
for skill in ceedling-test-from-diff fff-mock-generation c-code-review-from-diff change-spec-from-diff docx-review-comment-from-diff; do
  mkdir -p ~/.claude/skills/$skill
  cp skills/$skill/SKILL.md ~/.claude/skills/$skill/SKILL.md
done
```

インストール後、Claude Code を再起動するとスキルが認識されます。

---

## 各スキルの使い方

### `ceedling-test-from-diff`

```
git diff の変更を確認してテストを作成してください
```

変更された C 関数を解析し、Unity + FFF を使った `test_<module>.c` を生成または更新します。

### `fff-mock-generation`

```
git diff の変更に対して FFF フェイクを生成してください
```

外部関数呼び出しを検出し、`FAKE_VOID_FUNC` / `FAKE_VALUE_FUNC` 宣言と `setUp` の `RESET_FAKE` を生成します。

### `c-code-review-from-diff`

```
git diff HEAD~1 HEAD のコードレビューをしてください
```

以下の観点でレビューレポート（`docs/reviews/` 以下の Markdown）を生成します：

- **組み込み固有**: ISR 内の malloc・ブロッキング呼び出し、volatile 漏れ、HAL 境界違反、タイムアウトなし
- **一般品質**: 関数サイズ・ネスト深さ・戻り値未チェック・マジックナンバー

### `change-spec-from-diff`

```
git diff HEAD~1 HEAD の変更仕様書を作成してください
```

関数追加・インターフェース変更・動作変更などを分類し、互換性への影響と関連テストをまとめた Markdown を `docs/changes/` に出力します。

### `docx-review-comment-from-diff`

```
git diff HEAD~1 HEAD の内容で docs/spec.docx のどこを修正すればよいか教えてください
```

python-docx または pandoc で docx を解析し、変更されたシンボルに対応する仕様書の章・段落を特定して修正指示レポートを `docs/docx-review/` に出力します。

---

## 動作環境

| ツール | 用途 | 必須 |
| ------ | ---- | ---- |
| [Claude Code](https://claude.ai/code) | スキルの実行環境 | 必須 |
| [Ceedling](https://github.com/ThrowTheSwitch/Ceedling) | `ceedling-test-from-diff` のテスト実行 | `ceedling-test-from-diff` 使用時 |
| [FFF](https://github.com/meekrosoft/fff) (`fff.h`) | フェイク関数フレームワーク | `fff-mock-generation` 使用時 |
| python-docx または pandoc | docx テキスト抽出 | `docx-review-comment-from-diff` 使用時 |

---

## ライセンス

[LICENSE](LICENSE) を参照してください。
