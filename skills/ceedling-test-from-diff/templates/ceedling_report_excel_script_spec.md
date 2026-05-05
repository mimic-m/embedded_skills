# ceedling_report_excel.py Script Specification

Ceedling実行後のJUnit XML、gcovr JSON、Valgrind XML、テストコード内の日本語Doxygen範囲マーカーを入力として、単体テスト結果Excelを生成する後処理スクリプトの仕様テンプレートです。

## Purpose

`tools/ceedling_report_excel.py` はCeedling本体には組み込まず、テスト実行後の証跡ファイルを読み込んでExcelワークブックと証跡zipを生成する。

## Command Line

```bash
python tools/ceedling_report_excel.py \
  --mode diff \
  --template templates/unit_test_report_template.xlsx \
  --junit build/artifacts/test/report.xml \
  --coverage build/reports/gcovr.json \
  --coverage-reasons coverage_uncovered_reasons.yml \
  --memory build/reports/valgrind \
  --out build/reports/unit_test_report.xlsx
```

### Options

| Option | Required | Default | Description |
|---|---:|---|---|
| `--mode {diff,regression}` | no | `diff` | `diff`はgit diff対象ソースに対応するテストファイルを対象にする。`regression`は全`test/test_*.c`を対象にする。 |
| `--template PATH` | no | none | 既存Excelテンプレート。未指定時は必須シートだけを新規作成する。 |
| `--junit PATH` | yes | none | CeedlingのJUnit XML。テスト成功/失敗の唯一の入力元。 |
| `--coverage PATH` | yes | none | gcovr JSON。C0/C1集計と非通過箇所の入力元。 |
| `--coverage-reasons PATH` | no | none | カバレッジ非通過理由YAML。未指定または理由不足でもExcel生成は継続する。 |
| `--memory PATH` | yes | none | Valgrind Memcheck XMLファイル、またはXMLファイル群を含むディレクトリ。 |
| `--out PATH` | yes | none | 出力Excelファイル。 |
| `--source-root PATH` | no | `src` | ソースコードルート。 |
| `--test-root PATH` | no | `test` | テストコードルート。 |
| `--evidence-zip PATH` | no | `<out-dir>/unit_test_evidence.zip` | 添付相当の証跡zip出力先。 |
| `--embed-ole` | no | false | 証跡zipをOLE PackageとしてExcelへ埋め込む。Windows Excel COMは禁止。純Pythonでテンプレート内のプレースホルダOLEを差し替える。 |
| `--ole-placeholder NAME` | no | `unit_test_evidence.zip` | 差し替え対象のOLE Package表示名。テンプレート内に同名または既定位置のプレースホルダが必要。 |

## Target Selection

### diff mode

1. `git diff --name-only` で変更されたproduction `.c` ファイルを抽出する。
2. `src/<module>.c` から `test/test_<module>.c` を導出する。
3. 導出した各テストファイル内の全テストケースをExcel出力対象にする。

### regression mode

1. `test-root` 配下の `test_*.c` を全て対象にする。
2. 各テストファイル内の全テストケースをExcel出力対象にする。

## Source/Test Mapping

テストファイルとソースファイルは1対1とする。

| Test file | Source file |
|---|---|
| `test/test_foo.c` | `src/foo.c` |
| `test/sub/test_bar.c` | `src/bar.c` unless project-specific mapping is added later |

初期版では複雑なサブディレクトリ対応は行わない。対応不能な場合は明示エラーにする。

## Test Case Function Parsing

対象テスト関数は以下の形式に限定する。

```c
void test_<target_function>_<Success|Fail>_<representative_condition>(void)
```

Parsing:

1. `test_` の直後から最初の `_Success_` または `_Fail_` の直前までを `target_function` とする。
2. `Success` または `Fail` を正常系/異常系情報とする。
3. delimiter以降を `representative_condition` とする。
4. Excelの代表条件欄では `_` を半角スペースへ変換する。

Error:

- `_Success_` / `_Fail_` delimiterがない場合はエラー。
- JUnit XMLに同名テストケースがない場合はエラー。
- production関数名自体に `_Success_` または `_Fail_` が含まれる場合は曖昧なのでエラー、または将来のalias設定で対応する。

## Test Case Doxygen Comment Parsing

各テストケース関数の直前には、以下のDoxygenコメントが必須。

```c
/**
 * @brief <target_function> <Success|Fail>
 * Representative: <main test parameters and mock return values>
 * @details <expected result when executing the target function>
 */
void test_<target_function>_<Success|Fail>_<representative_condition>(void)
```

Extraction:

1. `@brief` 行からテスト対象関数名と `Success` / `Fail` を取得する。
2. `@brief` の次行から代表的なテストパラメータおよびmock関数戻り値を取得する。
3. `@details` から期待値を取得する。例: `xxを返す`, `yyにzzzが設定される`, `mmm関数がコールされること`。
4. 関数名から得た `target_function` / `Success|Fail` と `@brief` の内容が矛盾する場合はエラー。
5. `Representative:` ラベルは推奨。ラベルがない場合も、`@brief` 直後の1行を代表条件として扱う。

## Marker Parsing

各テストケース関数は以下3組のマーカーを必ず持つ。

```c
/** @verbatim テストパラメータ
 * @endverbatim
 */
/* code copied to Excel */
/** @verbatim テストパラメータ終了
 * @endverbatim
 */

/** @verbatim テストシーケンス
 * @endverbatim
 */
/* code copied to Excel */
/** @verbatim テストシーケンス終了
 * @endverbatim
 */

/** @verbatim チェック項目
 * @endverbatim
 */
/* code copied to Excel */
/** @verbatim チェック項目終了
 * @endverbatim
 */
```

Extraction rule:

1. 開始マーカーは `@verbatim <label>` と `@endverbatim` を含むDoxygenコメントブロック全体。
2. 終了マーカーは `@verbatim <label>終了` と `@endverbatim` を含むDoxygenコメントブロック全体。
3. 開始マーカーの閉じ `*/` の直後から、終了マーカーの開始 `/**` の直前までを抽出する。
4. マーカーコメント自体はExcelに入れない。
5. 抽出範囲の先頭/末尾の空行だけを削る。内部の改行、インデント、コメントは保持する。
6. 空範囲はエラー。コメントのみの範囲は、セットアップ不要など意図が読める場合のみ許容する。

Missing any marker pair is an error.

## JUnit XML

Input:

- Ceedlingが出力するJUnit XML。

Required fields:

- test case name
- failure/error presence
- optional message

Result mapping:

| XML condition | Excel テスト結果 |
|---|---|
| no `failure` and no `error` | `Success` |
| has `failure` or `error` | `Fail` |

Do not parse Ceedling stdout/stderr for pass/fail.

## gcovr JSON

Input:

```bash
gcovr --root . \
      --filter src/<module>.c \
      --branches \
      --json build/reports/gcovr.json \
      --fail-under-line 100 \
      --fail-under-branch 100 \
      --print-summary
```

Coverage output:

- file-level C0 line coverage
- file-level C1 branch coverage
- overall C0/C1
- uncovered lines
- uncovered branches

The script should support the gcovr JSON schema by feature detection rather than assuming a single gcovr version when practical.

## Coverage Reasons YAML

Input example:

```yaml
uncovered:
  - file: src/foo.c
    function: process
    type: C1
    line: 42
    reason: "防御的分岐であり、通常API経由では到達不可"
    action: "レビューにより妥当性確認"
```

Rules:

- Match by `file`, `type`, and `line`. Use `function` as additional context when available.
- Missing reason/action must not fail Excel generation.
- Unmatched stale YAML entries should be reported as warnings.
- Excelの `非通過理由` / `対応方針` は未入力なら空欄にする。

## Valgrind XML

Input command:

```bash
mkdir -p build/reports/valgrind
valgrind \
  --leak-check=full \
  --show-leak-kinds=all \
  --track-origins=yes \
  --xml=yes \
  --xml-file=build/reports/valgrind/test_<module>.xml \
  --error-exitcode=1 \
  build/test/out/test_<module>.out
```

Input path:

- one XML file, or
- a directory containing `*.xml`.

Result mapping:

| Valgrind condition | Excel result |
|---|---|
| no errors and no leaks | `Success` |
| any error/leak entry | `Fail` |

## Workbook Sheets

The workbook must contain:

- 表紙
- 変更履歴
- テスト方針
- テスト結果サマリ
- 各テストケースの情報
- カバレッジ結果
- メモリリークチェック結果

## Sheet: テスト結果サマリ

Columns/fields:

- テスト対象
- テスト実施日
- テスト結果サマリ
- カバレッジ結果サマリ
- メモリリークチェック結果サマリ
- 証跡zipファイル名
- 証跡zip作成日時
- 証跡zip内ファイル一覧

## Sheet: 各テストケースの情報

Columns:

| Column | Source |
|---|---|
| ソースコード | mapped source file path |
| テスト関数 | `@brief` and function-name parsed `target_function` |
| テストケース関数名 | C test function name |
| 正常系/異常系情報 | `@brief` and function-name parsed `Success` or `Fail` |
| 代表的なテストパラメータ及びmock関数の戻り値 | Doxygen representative line; fallback to function-name representative condition with `_` converted to spaces |
| 期待値 | Doxygen `@details` text |
| テストパラメータ | marker-extracted code |
| テストシーケンス | marker-extracted code |
| チェック項目 | marker-extracted code |
| テスト結果 | JUnit XML result |

## Sheet: カバレッジ結果

Coverage summary table:

- ファイル
- C0
- C1
- 行数
- カバー行数
- 分岐数
- カバー分岐数

Uncovered-location table:

- No
- ファイル
- 関数
- 種別 (`C0` or `C1`)
- 行番号
- 非通過コードまたは条件
- 非通過理由
- 対応方針

## Sheet: メモリリークチェック結果

Columns:

- テスト対象
- 種別
- 結果
- サイズ
- 件数
- 発生箇所
- 詳細

## Evidence Zip

Create `unit_test_evidence.zip` beside the workbook unless `--evidence-zip` is specified.

Include:

- JUnit XML
- gcovr JSON
- Valgrind XML file(s)
- coverage reasons YAML when present
- optional raw logs or HTML coverage reports when explicitly provided by future options

Initial implementation should always write the zip path and included file list to Excel. When `--embed-ole` is specified, it should also embed the zip as an OLE Package using the pure-Python placeholder replacement flow below.

## OLE Zip Embedding

Do not use Windows Excel COM automation.

The supported approach is template-based OOXML/OLE replacement:

1. The `.xlsx` template must already contain one embedded OLE Package placeholder on the `テスト結果サマリ` sheet.
2. The placeholder should be inserted manually when preparing the template, using a dummy zip file named `unit_test_evidence.zip`.
3. The script opens the `.xlsx` as a zip container.
4. It locates the target `/xl/embeddings/oleObject*.bin` referenced by the summary sheet drawing/relationship, or by the configured `--ole-placeholder` metadata when available.
5. It opens the OLE compound document and finds the `\x01Ole10Native` stream.
6. It replaces the Ole10Native payload with the generated evidence zip bytes while preserving the surrounding workbook relationships and drawing anchor.
7. It writes a new `.xlsx` package.

Constraints:

- The script must not call Excel, LibreOffice, or GUI automation.
- If the placeholder OLE stream is too small for the generated zip and the implementation cannot rebuild the compound file safely, fail with a clear error that says to create a larger placeholder zip in the template.
- If the template has no OLE placeholder, fail `--embed-ole` with a clear error. The workbook and evidence zip should still be produced without embedded OLE when practical.
- If `--template` is omitted, `--embed-ole` is invalid because a new workbook has no OLE placeholder or drawing relationship to replace.
- The embedded object display name should remain `unit_test_evidence.zip`.

Recommended implementation detail:

- Prefer a small dedicated helper such as `replace_ole10native_payload(xlsx_path, ole_name, payload_path, output_path)`.
- Use standard `zipfile` for the `.xlsx` package.
- Use an OLE compound file library only for reading and updating the existing `Ole10Native` stream.
- Do not generate fresh OOXML drawing anchors in the initial implementation; require the template placeholder.

## Error and Warning Policy

Errors, exit non-zero:

- required input file missing
- no target test files found
- test function name violates required convention
- required marker pair missing
- marker extraction range is empty
- JUnit XML has no result for a target test case
- gcovr JSON cannot be parsed
- Valgrind XML cannot be parsed
- output workbook cannot be written

Warnings, continue:

- coverage reason entry has no matching current uncovered item
- coverage reason/action is blank
- template sheet exists but lacks expected headings, if the script can still append a table
- optional evidence file is missing

## Recommended Implementation Phases

1. Parse `test_*.c` and output per-test-case JSON to stdout.
2. Parse JUnit XML and join test results.
3. Parse gcovr JSON and generate coverage tables.
4. Parse Valgrind XML and generate memory tables.
5. Generate workbook with `openpyxl`.
6. Create evidence zip.
7. Add template workbook support.
8. Add pure-Python template-based OLE placeholder replacement for `--embed-ole`.
