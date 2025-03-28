## センサーAPIドキュメント

### 概要

本APIは、センサーデータを受け取り、それを元に音楽生成のためのピッチデータを計算し、最終的にOpenAI APIを使用してMIDIノートシーケンスを生成します。また、センサーデータをセッションに記録し、統計情報を管理します。

---

## エンドポイント

### 1. センサーデータの送信

#### `POST /api/sensor`

**説明**  
センサーデータを受け取り、各データポイントに基づいてピッチデータを計算し、OpenAI APIを使用してMIDIノートシーケンスを生成します。さらに、関連するセッションのデータを更新します。

**リクエストボディ（JSON）**

```json
{
  "session_id": 1,
  "sensor_data": [
    { "vibration": 3.2, "pressure_avg": 6.2, "pitch_left": 0.3, "yaw_up": 0.1 },
    { "vibration": 4.1, "pressure_avg": 6.3, "pitch_left": 0.2, "yaw_up": 0.0 }
  ]
}
```

**レスポンス（JSON）**

```json
{
  "status": "OK",
  "session": {
    "id": 1,
    "total_vibration": 7.3,
    "pressure_avg": 6.25,
    "pitch_left": 0.5,
    "yaw_up": 0.1
  }
}
```

**エラーレスポンス**

- **400 Bad Request**: リクエストフォーマットが正しくない場合
- **404 Not Found**: 指定された `session_id` のセッションが存在しない場合
- **500 Internal Server Error**: サーバー内部でエラーが発生した場合

```json
{
  "error": "Session not found"
}
```

---

## データ処理の流れ

1. `sensor_data` の各データポイントを取得。
2. 各データに対し、以下の計算を実施：
   - `vibration` の合計値を計算。
   - `pressure_avg` の合計値を計算。
   - `pitch_left` の合計値を計算。
   - `yaw_up` の合計値を計算。
3. 指定された `session_id` に対応するセッションデータを取得。
4. 計算結果を既存のセッションデータに追加し、データを更新。
5. 更新されたセッションデータをレスポンスとして返す。

---

## 注意事項

- `session_id` は有効なセッションのIDである必要があります。
- `sensor_data` は適切な範囲の値を持つことが推奨されます。
- `OPENAI_API_KEY` が環境変数に設定されている必要があります（音楽生成を行う場合）。

---

## 例: `curl` コマンドによるリクエスト

```sh
curl -X POST http://localhost:3001/api/sensor \
  -H "Content-Type: application/json" \
  -d '{
    "session_id": 1,
    "sensor_data": [
      { "vibration": 3.2, "pressure_avg": 6.2, "pitch_left": 0.3, "yaw_up": 0.1 },
      { "vibration": 4.1, "pressure_avg": 6.3, "pitch_left": 0.2, "yaw_up": 0.0 }
    ]
  }'
```
