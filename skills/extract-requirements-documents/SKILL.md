---
name: extract-requirements-documents
description: "Use when extracting requirements from customer-provided documents (PDF, docx, xlsx) and generating a structured requirements definition document (要件定義書) in Japanese Markdown. Triggered by requests to analyze customer documents, extract requirements, or create a requirements definition from PDF/Word/Excel files."
---

# 要件定義書生成スキル

## 概要

顧客から受領した資料（PDF・docx・xlsx）からテキストを抽出し、要件を分類・整理して、日本語の要件定義書（Markdown）を生成する。

入力資料は1種類でも複数の組み合わせでもよい。抽出した内容をもとに以下の4カテゴリに分類する：

| カテゴリ | 説明 |
|----------|------|
| 機能要件 | システムが「何をするか」を定める要件 |
| 非機能要件 | 性能・信頼性・セキュリティなど「どうあるか」を定める要件 |
| 制約条件 | 技術・法規制・予算・スケジュールなど変更不可な制限 |
| 前提条件 | 本プロジェクトが成立するために事前に満たされているべき条件 |

## 呼び出し方

以下の情報を提供する：

1. **入力資料のパス** — 絶対パスまたはリポジトリルートからの相対パス。複数ファイル可。
   - 例: `docs/customer/rfq.pdf`, `specs/requirement.docx`, `data/items.xlsx`
2. **プロジェクト名**（任意）— 不明な場合は資料のファイル名から推測する

プロンプト例：
> 「`docs/rfq.pdf` と `specs/requirement.docx` から要件定義書を作成してください」

## 前提ツール確認

処理を開始する前に、各ファイル種別に対応するツールが利用可能か確認する。

### PDF

```bash
# 選択肢1: pdftotext (poppler-utils)
pdftotext --version 2>/dev/null | head -1 || echo "pdftotext not found"

# 選択肢2: pdfplumber (Python)
python3 -c "import pdfplumber; print('pdfplumber ok')" 2>/dev/null || echo "pdfplumber not found"
```

PDFファイルが入力に含まれ、どちらも利用不可の場合は処理を中止し、以下を表示する：

```
ERROR: PDF extraction tool not found.
Install one before proceeding:
  sudo apt install poppler-utils        (pdftotext — Debian/Ubuntu)
  brew install poppler                  (pdftotext — macOS)
  pip install pdfplumber                (Python alternative)
```

### docx

```bash
# 選択肢1: python-docx
python3 -c "import docx; print('python-docx ok')" 2>/dev/null || echo "python-docx not found"

# 選択肢2: pandoc
pandoc --version 2>/dev/null | head -1 || echo "pandoc not found"
```

docxファイルが入力に含まれ、どちらも利用不可の場合は処理を中止し、以下を表示する：

```
ERROR: docx extraction tool not found.
Install one before proceeding:
  pip install python-docx
  sudo apt install pandoc    (Debian/Ubuntu)
  brew install pandoc        (macOS)
```

### xlsx

```bash
# 選択肢1: openpyxl
python3 -c "import openpyxl; print('openpyxl ok')" 2>/dev/null || echo "openpyxl not found"

# 選択肢2: pandas
python3 -c "import pandas; print('pandas ok')" 2>/dev/null || echo "pandas not found"
```

xlsxファイルが入力に含まれ、どちらも利用不可の場合は処理を中止し、以下を表示する：

```
ERROR: xlsx extraction tool not found.
Install one before proceeding:
  pip install openpyxl
  pip install pandas openpyxl    (pandas requires openpyxl for xlsx)
```

## ワークフロー

```text
入力資料受領 → ツール確認 → テキスト抽出（種別ごと） → 要件分析・分類 → 要件定義書生成 → ファイル保存
```

---

### Step 0: 入力資料を確認する

提供されたファイルパスの存在と種別を確認する。

```bash
# ファイルの存在確認（各ファイルに対して実行）
ls -lh path/to/document.pdf
```

ファイルが存在しない場合はその旨を報告し、正しいパスを尋ねる。ファイルが存在する場合は拡張子から種別（PDF / docx / xlsx）を判定し、対応するツール確認（「前提ツール確認」セクション）に進む。

---

### Step 1: テキスト抽出

各ファイル種別に対して、利用可能なツールでテキストを抽出する。

#### PDF の抽出

```bash
# 選択肢1: pdftotext（レイアウト保持）
pdftotext -layout doc.pdf -

# 選択肢2: pdfplumber（Python）
python3 - doc.pdf << 'EOF'
import pdfplumber, sys
with pdfplumber.open(sys.argv[1]) as pdf:
    for i, page in enumerate(pdf.pages, 1):
        text = page.extract_text()
        if text:
            print(f"=== Page {i} ===")
            print(text)
EOF
```

抽出後、テキストが極端に少ない場合はスキャン画像ベースのPDFである可能性がある：

```bash
# 抽出テキストの文字数を確認する
pdftotext -layout doc.pdf - | wc -c
```

抽出文字数が **200文字未満** の場合は、OCRが必要な可能性が高い。処理を中止し、以下を表示する：

```
WARNING: Extracted fewer than 200 characters from the PDF file.
The document may be image-based (scanned). Run OCR before proceeding:
  pip install ocrmypdf
  ocrmypdf --skip-text input.pdf ocr-output.pdf
  # その後 ocr-output.pdf を入力として再実行する
```

#### docx の抽出

```bash
# 選択肢1: python-docx（見出しコンテキスト付き — 推奨）
python3 - spec.docx << 'EOF'
import docx, sys
import docx.text.paragraph, docx.table
doc = docx.Document(sys.argv[1])
current_heading = "(preamble)"
for block in doc.element.body:
    tag = block.tag.split('}')[-1]
    if tag == 'p':
        p = docx.text.paragraph.Paragraph(block, doc)
        if p.style.name.startswith("Heading"):
            current_heading = p.text.strip()
        elif p.text.strip():
            print(f"[{current_heading}] {p.text.strip()}")
    elif tag == 'tbl':
        tbl = docx.table.Table(block, doc)
        for row in tbl.rows:
            cells = [c.text.strip() for c in row.cells if c.text.strip()]
            if cells:
                print(f"[{current_heading}] TABLE_ROW: {' | '.join(cells)}")
EOF

# 選択肢2: pandoc（プレーンテキスト）
pandoc spec.docx -t plain
```

#### xlsx の抽出

```bash
# 選択肢1: openpyxl
python3 - data.xlsx << 'EOF'
import openpyxl, sys
wb = openpyxl.load_workbook(sys.argv[1], data_only=True)
for sheet in wb.sheetnames:
    ws = wb[sheet]
    print(f"=== Sheet: {sheet} ===")
    for row in ws.iter_rows(values_only=True):
        cells = [str(c) for c in row if c is not None]
        if cells:
            print(" | ".join(cells))
EOF

# 選択肢2: pandas
python3 - data.xlsx << 'EOF'
import pandas as pd, sys
xl = pd.ExcelFile(sys.argv[1])
for sheet in xl.sheet_names:
    df = pd.read_excel(xl, sheet_name=sheet)
    print(f"=== Sheet: {sheet} ===")
    print(df.to_string())
EOF
```

複数ファイルがある場合は、すべてのファイルに対して抽出を実行し、抽出テキストを **ファイル名・ページ/シート情報とともに** 内部バッファに蓄積する。以降のステップはこのバッファ全体を対象とする。

---

### Step 2: 要件の分析と分類

抽出テキスト全体を読み、以下の判定基準で各記述を4カテゴリに分類する。

#### 機能要件（FR）の判定基準

以下のパターンを含む記述は機能要件候補とする：

| シグナル | 例 |
|----------|----|
| システムが〜する / 〜できる | 「システムはユーザー認証を行う」 |
| 〜を提供する / 〜を送信する | 「データをCSVで出力する」 |
| 〜した場合に〜する（条件＋動作） | 「エラー発生時にアラートを表示する」 |
| ユースケース・シナリオ記述 | 「ユーザーがボタンを押すと…」 |
| 〜機能 / 〜処理 / 〜管理 | 「ログ管理機能」 |

#### 非機能要件（NFR）の判定基準

| シグナル | 分類 | 例 |
|----------|------|----|
| 応答時間・スループット・処理速度 | 性能 | 「3秒以内に応答する」 |
| 稼働率・MTBF・MTTR | 信頼性 | 「99.9%以上の可用性」 |
| 認証・暗号化・アクセス制御 | セキュリティ | 「TLS 1.2以上で通信する」 |
| テスト容易性・変更容易性 | 保守性 | 「モジュール単位でテスト可能」 |
| OS・言語・フレームワーク対応 | 移植性 | 「Linux/Windows両対応」 |
| 同時接続数・データ量 | スケーラビリティ | 「1000ユーザー同時接続」 |
| 障害復旧・フェイルオーバー | 可用性 | 「障害から5分以内に復旧」 |

#### 制約条件（CON）の判定基準

| シグナル | 分類 | 例 |
|----------|------|----|
| 使用技術・言語・フレームワーク指定 | 技術 | 「言語はC言語のみ使用」 |
| 法律・規格・標準への準拠 | 法規制 | 「ISO 26262に準拠」 |
| 予算・コスト上限 | 予算 | 「開発費は〇〇万円以内」 |
| 納期・マイルストーン | スケジュール | 「2026年3月末までに納品」 |
| 外部システム・プロトコル仕様 | インターフェース | 「既存REST APIと互換性を保つ」 |

#### 前提条件（PRE）の判定基準

| シグナル | 例 |
|----------|----|
| 〜が既に存在する / 〜は提供済み | 「顧客側でサーバーを用意済み」 |
| 〜は対象外 / 〜は含まない | 「ハードウェア設計は本スコープ外」 |
| 〜を前提とする / 〜が成立する前提で | 「ネットワーク接続が安定していることを前提とする」 |
| 除外事項・スコープ外 | 「多言語対応は含まない」 |

#### 分類の優先順位と重複処理

1つの記述が複数カテゴリに該当する場合は、**最も具体的なカテゴリ**を優先する（機能要件 > 非機能要件 > 制約条件 > 前提条件の順で具体性が高い）。

分類が困難な記述は要件定義書の「未確認事項・要確認（TBD）」セクションに記録する。

各要件には必ず出典（ファイル名・ページ番号またはシート名・見出し）を付記する。

#### 複数資料間の重複要件の統合ルール

同一要件が複数の入力資料に登場する場合は以下のルールで処理する：

| 状況 | 対処 |
|------|------|
| 複数資料で同一内容の要件が記載されている | IDを1つだけ採番し、「出典」列にすべての資料名・ページを列挙する（例: `rfq.pdf p.3, spec.docx §2.1`） |
| 複数資料で同一要件の記述が微妙に異なる（表現の違い） | より具体的・詳細な記述を本文として採用し、出典を複数列挙する |
| 複数資料で同一要件が**矛盾している**（数値・条件が異なる） | 両方の記述を抽出し、TBD-XXXとして「〈資料A〉と〈資料B〉で記述が矛盾。確認が必要」と記録する |

---

### Step 3: 要件IDの採番と優先度の判定

#### ID採番規則

| カテゴリ | IDプレフィックス | 例 |
|----------|-----------------|-----|
| 機能要件 | FR- | FR-001, FR-002 |
| 非機能要件 | NFR- | NFR-001, NFR-002 |
| 制約条件 | CON- | CON-001, CON-002 |
| 前提条件 | PRE- | PRE-001, PRE-002 |
| 未確認事項 | TBD- | TBD-001, TBD-002 |

番号は各カテゴリ内で出現順に001から採番する。

#### 優先度の判定基準

| 優先度 | 判定基準 |
|--------|----------|
| 高 | 「必須」「必ず」「〜しなければならない」「MUST」など強制表現、またはコアユースケースに直結 |
| 中 | 「望ましい」「できれば」「SHOULD」など推奨表現、または重要だが代替手段がある |
| 低 | 「あれば良い」「将来的に」「NICE」など任意表現、または補助的な機能 |

優先度が資料から読み取れない場合は「中」とし、「出典」列に「（推定）」と注記する。

---

### Step 4: 用語集の抽出

抽出テキスト中の以下を用語候補とする：

- 定義文を伴う専門用語（「〜とは〜のことをいう」「〜を指す」）
- 略語・頭字語（英字大文字3文字以上、または日本語略称）
- プロジェクト固有の概念名・製品名・システム名

用語集は資料を通じて統一する（同一概念に複数表記がある場合は正式表記を1つ選び、別名を括弧内に記す）。

---

### Step 5: 要件定義書を生成する

「出力テンプレート」セクションに従って日本語Markdownを生成する。

- 各テーブルの「〈〉」プレースホルダーを実際の内容で置き換える
- 内容が存在しないセクション（例: ステークホルダーが資料から不明）は削除せず、「資料から確認できず。要確認。」と記載してTBDに追記する
- 作成日は実行日を使用する

```bash
date +%Y-%m-%d
```

---

## 要件分析のガイドライン

### 抽出精度を上げるための読み方

**PDF・docx の場合:**
- 文書の章立て（見出し）を最初に把握し、各章の主題を理解してから段落を読む
- 表の各行は独立した要件候補として扱う（1行1要件を目安に）
- 箇条書きは親項目の文脈を継承する（「以下の条件を満たすこと：」の下の箇条書きはすべて要件）

**xlsx の場合:**
- シート名が要件カテゴリを示していることが多い（例: 「機能一覧」「性能要件」）
- 列ヘッダーが要件の属性（ID・要件名・説明・優先度）に対応する場合は、そのまま活用する
- 「備考」「コメント」列の内容は前提条件またはTBDの候補

### 曖昧な記述の処理

| 曖昧さの種類 | 対処 |
|-------------|------|
| 主語が不明確（「〜する」の主体が不明） | 文脈から推測し、「（推定: システム）」と注記する |
| 数値基準が未定義（「十分な性能」「高速に」） | NFRとして抽出し、TBDに「具体的な数値基準を確認する」を追加 |
| 条件が複数解釈できる | 最も広い解釈を採用し、TBDに代替解釈を記録する |
| 矛盾する記述が複数資料に存在する | 両方を抽出し、TBDで「〈資料A〉と〈資料B〉で記述が矛盾。確認が必要」と記録する |

---

## 出力テンプレート

```markdown
# 要件定義書

> **作成日:** YYYY-MM-DD
> **入力資料:** doc1.pdf, spec.docx, requirements.xlsx
> **バージョン:** 1.0

---

## 1. プロジェクト概要

〈プロジェクトの目的と背景を2〜3文で記述。入力資料の冒頭・目的セクションから抽出する〉

---

## 2. ステークホルダー

| 役割 | 名称・組織 | 責任範囲 |
|------|------------|----------|
| 〈顧客〉 | 〈記述〉 | 〈記述〉 |
| 〈開発〉 | 〈記述〉 | 〈記述〉 |

---

## 3. 機能要件

| ID | 要件名 | 説明 | 優先度 | 出典 |
|----|--------|------|--------|------|
| FR-001 | 〈機能名〉 | 〈説明〉 | 高/中/低 | 〈資料名 p.N〉 |

---

## 4. 非機能要件

| ID | 分類 | 要件名 | 説明 | 優先度 | 出典 |
|----|------|--------|------|--------|------|
| NFR-001 | 性能 | 〈要件名〉 | 〈説明〉 | 高/中/低 | 〈資料名〉 |

分類例: 性能, 信頼性, セキュリティ, 保守性, 移植性, 可用性, スケーラビリティ

---

## 5. 制約条件

| ID | 分類 | 制約内容 | 出典 |
|----|------|----------|------|
| CON-001 | 技術 | 〈制約内容〉 | 〈資料名〉 |

分類例: 技術, 法規制, 予算, スケジュール, インターフェース

---

## 6. 前提条件・除外事項

### 前提条件

| ID | 内容 | 出典 |
|----|------|------|
| PRE-001 | 〈前提〉 | 〈資料名〉 |

### 除外事項

- 〈対象外の事項〉

---

## 7. 用語集

| 用語 | 定義 |
|------|------|
| 〈用語〉 | 〈定義〉 |

---

## 8. 未確認事項・要確認

| ID | 質問・確認事項 | 関係者 | 期限 |
|----|--------------|--------|------|
| TBD-001 | 〈確認が必要な事項〉 | 〈担当〉 | 〈期限〉 |
```

---

## 出力ファイル

`docs/requirements/YYYY-MM-DD-requirements.md` に保存する。リポジトリルート（`.git/` が存在するディレクトリ）からの相対パス。プロジェクト名が判明している場合は `YYYY-MM-DD-<プロジェクト名>-requirements.md` とする。出力前にディレクトリを作成する。

```bash
mkdir -p docs/requirements
```

---

## よくある誤り

| 誤り | 対処 |
|------|------|
| 機能要件と非機能要件を混同する（例: 「高速に処理する」を機能要件に分類） | 「何をするか」は機能要件、「どの程度か」は非機能要件。性能・品質の数値基準は常にNFR |
| 制約条件を非機能要件に分類する（例: 「C言語のみ使用」をNFRにする） | 変更不可な前提・外部から課される制限は制約条件。選択の余地がない点が判断基準 |
| 出典を省略する | すべての要件に「ファイル名 + ページ番号またはシート名」を付記する。後からトレーサビリティを確保するために必須 |
| 曖昧な記述を無視する | TBD-XXXとして記録する。無視・省略は不可 |
| テーブルのプレースホルダーを残したまま出力する | 「〈〉」を必ず実際のテキストで置き換える。情報が不明な場合は「（資料から確認できず）」と明記する |
| 複数ファイルの矛盾を見落とす | 同一概念が複数資料に出現する場合は必ず照合し、矛盾があればTBDに記録する |
| xlsxのシートを1枚だけ処理して残りを見落とす | `wb.sheetnames` または `xl.sheet_names` でシート一覧を取得し、全シートを処理する |
