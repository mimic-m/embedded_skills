---
name: static-analysis-report
description: "Use when generating a severity-annotated Markdown report from static analysis tool output (Cppcheck or PC-lint). Triggered by requests to review static analysis results, convert Cppcheck XML or PC-lint logs into a structured report, or identify MISRA-C violations with embedded-specific severity mapping."
---

# 静的解析レポート生成スキル

## 概要

Cppcheck または PC-lint の解析結果を読み込み、組み込み固有の重大度昇格ルールを適用したうえで、`[CRITICAL] / [HIGH] / [MEDIUM] / [LOW]` タグ付きの Markdown レポートを生成する。

`c-code-review-from-diff` スキルと同一の出力フォーマットを使用するため、2つのレポートを並べて参照できる。

## 呼び出し方

以下のいずれかの情報を提供する：

1. **既存の解析結果ファイルのパス** — Cppcheck XML / テキスト、PC-lint テキスト
2. **解析対象のソースディレクトリ** — Cppcheck をその場で実行する

プロンプト例：
> 「`cppcheck-result.xml` から静的解析レポートを作成してください」
> 「`src/` ディレクトリに Cppcheck を実行してレポートを作成してください」

## 前提ツール確認

解析結果ファイルが提供された場合はツール確認不要。ソースディレクトリが提供された場合は以下を確認する：

```bash
cppcheck --version 2>/dev/null || echo "cppcheck not found"
```

Cppcheck が見つからない場合は処理を中止し、以下を表示する：

```
ERROR: Cppcheck not found.
Install before proceeding:
  sudo apt install cppcheck        (Debian/Ubuntu)
  brew install cppcheck            (macOS)
  winget install Cppcheck.Cppcheck (Windows)
```

## ワークフロー

```text
入力判定 → 解析実行（必要時） → 結果パース → 重大度マッピング → 組み込み昇格ルール適用 → レポート生成 → ファイル保存
```

---

### Step 0: 入力を判定する

| 入力 | 処理 |
|------|------|
| `.xml` ファイルパスが提供された | Step 1a（XML パース）へ |
| `.txt` / `.log` ファイルパスが提供された | Step 1b（テキストパース）へ |
| ソースディレクトリが提供された | Step 1c（Cppcheck 実行）→ Step 1a へ |
| 入力なし | 「解析結果ファイルまたはソースディレクトリのパスを指定してください」と尋ねる |

---

### Step 1a: Cppcheck XML をパースする

```bash
python3 - result.xml << 'EOF'
import xml.etree.ElementTree as ET, sys
tree = ET.parse(sys.argv[1])
for error in tree.findall('.//error'):
    eid  = error.get('id', '')
    sev  = error.get('severity', '')
    msg  = error.get('msg', '')
    cwe  = error.get('cwe', '')
    loc  = error.find('location')
    f    = loc.get('file', '?') if loc is not None else '?'
    l    = loc.get('line', '?') if loc is not None else '?'
    print(f"{sev}|{eid}|{f}:{l}|{cwe}|{msg}")
EOF
```

出力形式: `severity|id|file:line|cwe|message`

---

### Step 1b: テキスト形式をパースする

**Cppcheck GCC テンプレート形式:**

```
src/uart.c:42:5: error: Uninitialized variable: x [uninitvar]
```

パターン: `^(.+):(\d+):\d+: (error|warning|style|performance|portability|information): (.+) \[(.+)\]$`

**PC-lint 形式:**

```
src/uart.c(42) : error 530: Symbol 'x' not initialized
src/uart.c(42) : warning 714: Symbol 'y' not referenced  [MISRA 2012 Rule 9.1, required]
```

パターン: `^(.+)\((\d+)\) : (error|warning|note|info) (\d+): (.+?)(?:\s+\[(.+)\])?$`

---

### Step 1c: Cppcheck をその場で実行する

最も正確な解析のために、利用可能なオプションを以下の優先順位で選択する：

```bash
# 優先1: compile_commands.json がある場合（ビルド設定をそのまま使用 — 最も正確）
cppcheck --project=compile_commands.json --xml --xml-version=2 --enable=all \
  2> /tmp/cppcheck-result.xml

# compile_commands.json がない場合は cmake で生成できる
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build && cp build/compile_commands.json .
```

```bash
# 優先2: 抑制ファイルがある場合（既知の偽陽性を除外）
cppcheck --xml --xml-version=2 --enable=all \
  --suppressions-list=suppressions.xml src/ 2> /tmp/cppcheck-result.xml
```

```bash
# 優先3: 基本実行
cppcheck --xml --xml-version=2 --enable=all --inconclusive src/ 2> /tmp/cppcheck-result.xml

# MISRA-C 2012 アドオンを追加する場合
cppcheck --xml --xml-version=2 --enable=all --addon=misra src/ 2> /tmp/cppcheck-result.xml

# インクルードパスを指定する場合
cppcheck --xml --xml-version=2 --enable=all -I include/ src/ 2> /tmp/cppcheck-result.xml
```

> **抑制ファイルについて:** プロジェクトに `suppressions.xml` や `.cppcheck` が存在する場合は必ず使用する。これらのファイルはチームが合意した既知の偽陽性を除外するためのものであり、無視するとレポートがノイズで埋まる。抑制ファイルが存在するか確認するには:
> ```bash
> ls suppressions.xml .cppcheck 2>/dev/null
> ```

実行後、生成された `/tmp/cppcheck-result.xml` を Step 1a で処理する。

---

### Step 2: 重大度をマッピングする

パースした各指摘を以下のルールで `[CRITICAL] / [HIGH] / [MEDIUM] / [LOW]` に変換する。

#### Cppcheck severity → スキル重大度（基本マッピング）

| Cppcheck severity | スキル重大度 |
|-------------------|------------|
| `error` | HIGH |
| `warning` | MEDIUM |
| `style` | LOW |
| `performance` | MEDIUM |
| `portability` | LOW |
| `information` | 出力しない（件数集計にのみ含める） |

#### MISRA-C 規則種別による上書き

指摘メッセージまたは ID に `misra` が含まれる場合、規則種別で重大度を決定する（基本マッピングより優先）：

| MISRA 規則種別 | スキル重大度 |
|--------------|------------|
| Mandatory | CRITICAL |
| Required | HIGH |
| Advisory | MEDIUM |

規則種別が不明な場合は以下の代表的ルールを参照する：

| ルール番号 | 種別 | 内容（概要） |
|-----------|------|------------|
| 1.3 | Mandatory | 未定義・処理系依存の動作 |
| 2.1 | Mandatory | 到達不能コード |
| 9.1 | Mandatory | 未初期化変数の使用 |
| 10.1–10.8 | Required | 演算オペランドの型 |
| 11.1–11.4 | Required | ポインタ変換 |
| 14.4 | Mandatory | if/while 条件式の型 |
| 15.5 | Required | return 文の位置 |
| 17.1 | Mandatory | 可変引数（`<stdarg.h>`）の使用 |
| 18.1 | Required | 配列境界外アクセス |
| 21.3 | Required | `<stdlib.h>` の malloc/free |
| 22.1–22.2 | Required | 動的メモリの解放 |

#### PC-lint severity → スキル重大度

| PC-lint severity | スキル重大度 |
|------------------|------------|
| `error` | HIGH |
| `warning` | MEDIUM |
| `note` / `info` | LOW |

MISRA タグ付きの場合は上記 MISRA 種別ルールを適用する。

---

### Step 3: 組み込み固有の重大度昇格ルールを適用する

以下の条件に該当する指摘は、重大度を **1段階上げる**（HIGH→CRITICAL、MEDIUM→HIGH、LOW→MEDIUM）：

| 昇格トリガー | 判定方法 |
|------------|--------|
| ISR ハンドラ内の指摘 | ファイル名・関数名に `IRQHandler`, `_Handler`, `_isr`, `_ISR` が含まれる、または `__attribute__((interrupt))` が関連する |
| ヒープ操作 | `malloc`, `free`, `new`, `delete`, `realloc`, `calloc` の使用 |
| volatile 漏れ | Cppcheck ID `unreadVariable`, `redundantAssignment`、または MISRA Rule 8.6/19.x |
| スタック上の大きな配列 | CWE-121（スタックバッファオーバーフロー）|
| 初期化されていない変数 | Cppcheck ID `uninitvar`, `uninitStructMember` |

昇格後に CRITICAL を超えることはない（上限は CRITICAL）。

---

### Step 4: レポートを生成する

指摘を重大度順（CRITICAL → HIGH → MEDIUM → LOW）に並べ、「出力フォーマット」テンプレートに従って Markdown を生成する。

重大度ごとの件数サマリーを冒頭テーブルに記載する。

指摘が0件のカテゴリはサマリーテーブルのみに `0` と記載し、「問題一覧」セクションには含めない。

#### 大量指摘時のフィルタリング

CRITICAL + HIGH の合計が **50件を超える場合**、またはMEDIUM以下を含めた総件数が **100件を超える場合** は、初回対応フェーズとして以下のように出力を絞る：

- **問題一覧**: CRITICAL と HIGH のみ記載する
- **サマリーテーブル**: MEDIUM / LOW は件数のみ記載（詳細は省略）
- レポートの冒頭に以下の注記を追加する：

```markdown
> **注記（初回対応フェーズ）:** 指摘件数が多いため、問題一覧には CRITICAL / HIGH のみを記載しています。
> MEDIUM / LOW はサマリー件数を参照してください。CRITICAL / HIGH をすべて解消した後、
> MEDIUM / LOW を対象に再実行することを推奨します。
```

CRITICAL + HIGH が50件以下の場合は、すべての重大度を通常通り出力する。

---

## 出力フォーマット

```markdown
# 静的解析レポート

> **解析日:** YYYY-MM-DD
> **ツール:** Cppcheck 2.13 / PC-lint 9.0
> **対象:** src/
> **オプション:** --enable=all --addon=misra

---

## サマリー

| 重大度 | 件数 |
|--------|------|
| CRITICAL | 2 |
| HIGH | 5 |
| MEDIUM | 8 |
| LOW | 3 |
| **合計** | **18** |

---

## 問題一覧

### [CRITICAL] uninitvar: 変数 'buf' が初期化されていない（ISR 内）

- **ファイル:** `src/uart.c:87`
- **ID:** `uninitvar` / CWE-457
- **昇格理由:** ISR ハンドラ（`USART1_IRQHandler`）内の指摘
- **メッセージ:** Uninitialized variable: buf
- **修正方針:** 宣言時に `NULL` または有効なアドレスで初期化する。

```c
/* 修正前 */
uint8_t *buf;
HAL_UART_Receive(&huart1, buf, RX_LEN, 100);

/* 修正後 */
uint8_t rx_buf[RX_LEN];
HAL_UART_Receive(&huart1, rx_buf, RX_LEN, 100);
```

---

### [CRITICAL] MISRA 2012 Rule 1.3 (Mandatory): 未定義動作

- **ファイル:** `src/sensor.c:34`
- **ID:** `misra-c2012-1.3`
- **メッセージ:** There is undefined behavior at operand of operator '<<' — shift count is negative or too large.
- **修正方針:** シフト量をビット幅未満に制限する。型の昇格に注意し `uint32_t` に明示的にキャストする。

---

### [HIGH] MISRA 2012 Rule 21.3 (Required): malloc の使用

- **ファイル:** `src/driver.c:55`
- **ID:** `misra-c2012-21.3`
- **メッセージ:** The Standard Library function 'malloc' shall not be used.
- **修正方針:** 静的メモリプールまたはスタックバッファを使用する。

---

### [MEDIUM] memleak: メモリリーク

- **ファイル:** `src/can.c:102`
- **ID:** `memleak` / CWE-401
- **メッセージ:** Memory leak: ptr
- **修正方針:** エラーパスで `free(ptr)` を呼び出す、または goto による一元的な解放を実装する。

---

## 確認済み（指摘なし）

以下の観点では問題が検出されなかった：

- [embedded] ISR 内の malloc/free
- [embedded] ISR 内のブロッキング呼び出し
- [misra   ] Rule 14.4 — if/while 条件式の型
- [quality ] ヌルポインタ参照（nullPointer）
- [quality ] 配列境界外アクセス（arrayIndexOutOfBounds）
```

---

## 出力ファイル

`docs/static-analysis/YYYY-MM-DD-<対象>-static-analysis.md` に保存する。リポジトリルート（`.git/` が存在するディレクトリ）からの相対パス。`<対象>` は解析したディレクトリ名またはファイルのベース名。

```bash
mkdir -p docs/static-analysis
```

---

## 重大度リファレンス

| 重大度 | 意味 | 対応 |
|--------|------|------|
| CRITICAL | 安全性リスク・データ破壊・未定義動作 | マージ前に必ず修正 |
| HIGH | バグまたは重大な品質問題 | マージ前に修正すべき |
| MEDIUM | 保守性の懸念 | 修正を検討 |
| LOW | スタイルまたは軽微な提案 | 任意 |

---

## よくある誤り

| 誤り | 対処 |
|------|------|
| `information` 指摘をレポートに含める | 件数集計にのみ反映し、問題一覧には出力しない |
| MISRA 規則種別を確認せず `style` → LOW にマッピングする | MISRA の Mandatory は必ず CRITICAL、Required は HIGH にする |
| 昇格ルールを適用せず基本マッピングのみ使用する | ISR ハンドラ内の指摘は必ず1段階昇格する |
| 0件のカテゴリのセクションを省略してサマリーにも記載しない | サマリーテーブルには件数 `0` を記載する。「問題一覧」セクションは省略してよい |
| PC-lint のメッセージ番号とメッセージ本文のみを記載し MISRA 参照を見落とす | `[MISRA ...]` タグが行末にある場合は必ず MISRA 種別ルールを適用する |
