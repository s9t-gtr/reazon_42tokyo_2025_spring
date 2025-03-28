## API Documentation - Sessions

### Base URL

```
/api/sessions
```

### 1. `GET /api/sessions`

#### 概要

全てのセッションデータを取得します。オプションで、`start_date` と `end_date` をパラメータとして指定することで、特定の期間内のデータを取得できます。

#### リクエストパラメータ

- **start_date** (オプション): 期間の開始日 (`YYYY-MM-DD` 形式)
- **end_date** (オプション): 期間の終了日 (`YYYY-MM-DD` 形式)

#### 例 1: 特定の期間のデータを取得

```
GET /api/sessions?start_date=2025-03-20&end_date=2025-03-25
```

#### 例 2: 全てのセッションデータを取得

```
GET /api/sessions
```

#### レスポンス

- **成功時 (200 OK)**: セッションのリストが返されます。

```json
[
  {
    "id": 1,
    "date": "2025-03-25",
    "duration": 120.5,
    "heltzh": 60.2,
    "yaw_up": 45.0,
    "pitch_left": 30.5,
    "pressure_avg": 1012.3,
    "overpressure_count": 5,
    "created_at": "2025-03-25T10:00:00.000Z",
    "updated_at": "2025-03-25T10:00:00.000Z"
  },
  {
    "id": 2,
    "date": "2025-03-24",
    "duration": 90.0,
    "heltzh": 50.0,
    "yaw_up": 40.0,
    "pitch_left": 20.0,
    "pressure_avg": 1010.5,
    "overpressure_count": 3,
    "created_at": "2025-03-24T10:00:00.000Z",
    "updated_at": "2025-03-24T10:00:00.000Z"
  }
]
```

- **エラーレスポンス (400 Bad Request)**: 不正なリクエストパラメータが提供された場合。

```json
{
  "error": "Invalid date format"
}
```

---

### 2. `GET /api/sessions/:id`

#### 概要

指定したIDのセッションデータを取得します。

#### リクエストパラメータ

- **id**: セッションのID（URLパラメータ）

#### 例: セッションID 1 のデータを取得

```
GET /api/sessions/1
```

#### レスポンス

- **成功時 (200 OK)**: セッションの詳細が返されます。

```json
{
  "id": 1,
  "date": "2025-03-25",
  "duration": 120.5,
  "heltzh": 60.2,
  "yaw_up": 45.0,
  "pitch_left": 30.5,
  "pressure_avg": 1012.3,
  "overpressure_count": 5,
  "created_at": "2025-03-25T10:00:00.000Z",
  "updated_at": "2025-03-25T10:00:00.000Z"
}
```

- **エラーレスポンス (404 Not Found)**: セッションが見つからない場合。

```json
{
  "error": "Session not found"
}
```

---

### 3. `POST /api/sessions`

#### 概要

新しいセッションを作成します。

#### リクエストボディ

- **date**: セッションの日付 (`YYYY-MM-DD` 形式)
- **duration**: セッションの時間（浮動小数点数）
- **heltzh**: セッションのヘルツ値（浮動小数点数）
- **yaw_up**: ヨー（上）の角度（浮動小数点数）
- **pitch_left**: ピッチ（左）の角度（浮動小数点数）
- **pressure_avg**: 平均圧力（浮動小数点数）
- **overpressure_count**: 過圧回数（整数）

#### 例: 新しいセッションの作成

```
POST /api/sessions
```

リクエストボディ:

```json
{
  "session": {
    "date": "2025-03-26",
    "duration": 100.5,
    "heltzh": 55.0,
    "yaw_up": 35.0,
    "pitch_left": 25.0,
    "pressure_avg": 1012.0,
    "overpressure_count": 4
  }
}
```

#### レスポンス

- **成功時 (201 Created)**: 作成されたセッションの詳細が返されます。

```json
{
  "id": 3,
  "date": "2025-03-26",
  "duration": 100.5,
  "heltzh": 55.0,
  "yaw_up": 35.0,
  "pitch_left": 25.0,
  "pressure_avg": 1012.0,
  "overpressure_count": 4,
  "created_at": "2025-03-26T10:00:00.000Z",
  "updated_at": "2025-03-26T10:00:00.000Z"
}
```

- **エラーレスポンス (422 Unprocessable Entity)**: リクエストが無効である場合（例えば、必須パラメータが欠けている場合）。

```json
{
  "date": ["can't be blank"],
  "duration": ["is not a number"],
  "heltzh": ["is not a number"]
}
```

---

### エラーハンドリング

全てのAPIエンドポイントでは、以下のようなエラーハンドリングが行われます。

- **404 Not Found**: リソースが見つからない場合（例: `GET /api/sessions/:id` で指定されたIDのセッションが存在しない場合）。
- **400 Bad Request**: 無効なリクエストパラメータが提供された場合（例: 無効な日付フォーマット）。
- **422 Unprocessable Entity**: リクエストが無効な場合（例: 必須パラメータが不足している）。

---

### 備考

- `date` パラメータは、`YYYY-MM-DD` 形式で指定してください。Railsは、これを自動的に `Date` 型に変換します。
- `start_date` と `end_date` は両方とも指定される必要があります。片方だけ指定されている場合は、エラーとして処理されるか、全データが返される場合があります。

---

このAPIドキュメントをもとに、適切なリクエストを送信することで、セッションデータを取得・作成することができます。
