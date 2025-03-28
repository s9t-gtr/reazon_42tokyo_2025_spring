# setup

.env.sampleを.envに置き換える

```sh
bundle install
rails s -p 3001
```

APIテスト

```sh
curl -X POST http://localhost:3001/api/sensor -H "Content-Type: application/json" -d '{"ax": 0.5, "ay": 0.3, "az": 0.8, "pressure": 1.2}'
```
