# センサーAPIドキュメント

## 概要

本APIは、センサーデータを受け取り、それを元に音楽生成のためのピッチデータを計算し、最終的にOpenAI APIを使用してMIDIノートシーケンスを生成します。

## エンドポイント

### 1. センサーデータの送信

#### `POST /api/sensor`

**説明**
センサーデータを受け取り、各データポイントに基づいてピッチデータを計算し、OpenAI APIを使用してMIDIノートシーケンスを生成します。

**リクエストボディ（JSON）**

```json
{
  "sensor_data": [
    { "time": 0.0, "ax": 0.5, "ay": 0.3, "az": 0.3, "pressure": 1.2 },
    { "time": 0.00125, "ax": 0.49, "ay": 0.31, "az": 0.29, "pressure": 1.22 },
    { "time": 0.0025, "ax": 0.51, "ay": 0.28, "az": 0.32, "pressure": 1.18 }
  ]
}
```

**レスポンス（JSON）**

```json
{
  "pitch_data": [
    {
      "time": 0.0,
      "pitch_factor": 0.678,
      "roll": 5.3,
      "pitch": -8.9,
      "pressure": 1.2,
      "instrument": "piano"
    }
  ],
  "generated_music": [
    {
      "pitch": 60,
      "velocity": 80,
      "duration": 0.5
    },
    {
      "pitch": 64,
      "velocity": 85,
      "duration": 0.4
    }
  ]
}
```

**エラーレスポンス**

- **400 Bad Request**: リクエストフォーマットが正しくない場合
- **500 Internal Server Error**: サーバー内部でエラーが発生した場合

```json
{
  "error": "Internal Server Error",
  "details": "param is missing or the value is empty or invalid: sensor_data"
}
```

## データ処理の流れ

1. `sensor_data` の各データポイントを取得。
2. 各データに対し、以下の計算を実施：
   - `pitch_factor`: 加速度のベクトル長
   - `roll`: `atan2(ay, az)` の計算結果を度数変換
   - `pitch`: `atan2(-ax, sqrt(ay^2 + az^2))` の計算結果を度数変換
3. OpenAI API に最近の10個のデータを元に音楽生成を依頼。
4. 生成されたMIDIデータをレスポンスとして返す。

## 注意事項

- `OPENAI_API_KEY` が環境変数に設定されている必要があります。
- OpenAI APIのレスポンスフォーマットが変更される可能性があります。
- センサーデータは適切な範囲の値を持つことが推奨されます。

## 例: `curl` コマンドによるリクエスト

```sh
curl -X POST http://localhost:3001/api/sensor \
  -H "Content-Type: application/json" \
  -d '{
    "sensor_data": [
      {"time": 0.000, "ax": 0.50, "ay": 0.30, "az": 0.30, "pressure": 1.20},
      {"time": 0.00125, "ax": 0.49, "ay": 0.31, "az": 0.29, "pressure": 1.22}
    ]
  }'
```
