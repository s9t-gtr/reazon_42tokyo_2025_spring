#include <WiFi.h>
#include <HTTPClient.h>
#include <cJSON.h>

// set pin numbers

// buttons
const int buttonPin = 14; // ボタンのピン番号
// 加速度センサー
const int xPin = 16; // X軸のピン番号
const int yPin = 17; // Y軸のピン番号
const int zPin = 18; // Z軸のピン番号
int buttonState = 0;     // 現在のボタンの状態
int lastButtonState = 0; // 前回のボタンの状態

const int ledPin = 36;     // LED pin
const int volPin = 2;//圧力センサ用アナログピン

const char* ssid = "hi_guest";
const char* password = "guestP@sS";
const char* url = "http://abehiroshi.la.coocan.jp/top.htm";

////////////////////
// 加速度センサー //
// KXR94-2050     //
////////////////////

// センサー特性値
const float vRef = 3.3;       // リファレンス電圧 (ESP32は3.3V)
const float zeroG = 1.65;     // 0Gのときの電圧 (V) - 通常は供給電圧の半分
const float sensitivity = 0.66; // 感度 (V/g) - KXR94-2050のデータシートより

// ローパスフィルター用の係数
const float alpha = 0.2; // フィルター係数（0〜1の値、小さいほど強くフィルタリング）

// 角度計算用変数
float filteredAccX = 0, filteredAccY = 0, filteredAccZ = 0;

// 加速度計算用変数
unsigned long lastTime = 0;
float lastAccX = 0, lastAccY = 0, lastAccZ = 0;
float accelerationMagnitude = 0; // 加速度の大きさ
boolean isFirstReading = true;

// 波の回数（振動数）計測用変数
unsigned long lastCountTime = 0;  // 最後にカウントをリセットした時間
int vibrationCount = 0;          // 振動回数カウンター
float lastAccMagnitude = 0;      // 前回の加速度値
boolean wasAboveThreshold = false; // 閾値を超えたかのフラグ
const float VIBRATION_THRESHOLD = 0.1; // 振動と判定する閾値 (m/s²) - 調整が必要かも

//////////
// 統計処理用の変数 //
//////////

// データ記録用の変数とバッファサイズ
const int BUFFER_SIZE = 120; // 2分間のデータ (1秒1回 × 120秒)

// 角度データ用のバッファ
float rollHistory[BUFFER_SIZE];
float pitchHistory[BUFFER_SIZE];

// 振動数データ用のバッファ
int vibrationsHistory[BUFFER_SIZE];

// 圧力データ用のバッファ
int pressureHistory[BUFFER_SIZE];

// 統計データ格納用の構造体
struct SensorStats {
  // 角度の統計
  float rollAvg;
  float rollMax;
  float rollMin;
  float rollStdDev;
  
  float pitchAvg;
  float pitchMax;
  float pitchMin;
  float pitchStdDev;
  
  // 振動の統計
  float vibrationAvg;
  int vibrationMax;
  int totalVibrations;
  
  // 圧力の統計
  float pressureAvg;
  int pressureMax;
  int pressureMin;
};

SensorStats stats; // 統計結果を保存する変数

// バッファの現在位置と有効データ数
int bufferIndex = 0;
int validDataCount = 0;

// 統計データを更新する間隔 (ミリ秒)
const unsigned long STATS_UPDATE_INTERVAL = 10000; // 10秒ごとに更新
unsigned long lastStatsUpdateTime = 0;

// 統計結果の送信間隔 (ミリ秒)
const unsigned long STATS_SEND_INTERVAL = 60000; // 60秒ごとに送信
unsigned long lastStatsSendTime = 0;

//////////
//////////

void setup() {
  Serial.begin(115200);
  delay(4000); // シリアルモニタの準備待ち

  // ADCの解像度設定（ESP32は12ビットADC）
  analogReadResolution(12);

  // initialize the button pin as an input
  pinMode(buttonPin, INPUT);
  // initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);
  // initialize the Pressure Sensor pin as an output
  pinMode(volPin, INPUT);

  // 統計データの初期化
  initializeStats();
  
  // 最初のキャリブレーション実行
  calibrateSensor();

  // Wi-Fi接続開始
  Serial.printf("Wi-Fiに接続中: %s\n", ssid);
  WiFi.begin(ssid, password);

  // 接続が確立されるまで待機
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fiに接続完了");
  Serial.print("IPアドレス: ");
  Serial.println(WiFi.localIP());
}

// 統計データの初期化
void initializeStats() {
  stats.rollAvg = 0;
  stats.rollMax = -1000;
  stats.rollMin = 1000;
  stats.rollStdDev = 0;
  
  stats.pitchAvg = 0;
  stats.pitchMax = -1000;
  stats.pitchMin = 1000;
  stats.pitchStdDev = 0;
  
  stats.vibrationAvg = 0;
  stats.vibrationMax = 0;
  stats.totalVibrations = 0;
  
  stats.pressureAvg = 0;
  stats.pressureMax = 0;
  stats.pressureMin = 4095; // 12ビットADCの最大値
  
  // バッファをクリア
  for (int i = 0; i < BUFFER_SIZE; i++) {
    rollHistory[i] = 0;
    pitchHistory[i] = 0;
    vibrationsHistory[i] = 0;
    pressureHistory[i] = 0;
  }
  
  bufferIndex = 0;
  validDataCount = 0;
}

// データをバッファに追加する関数
void addDataToBuffer(float roll, float pitch, int vibrations, int pressure) {
  // バッファに新しいデータを追加
  rollHistory[bufferIndex] = roll;
  pitchHistory[bufferIndex] = pitch;
  vibrationsHistory[bufferIndex] = vibrations;
  pressureHistory[bufferIndex] = pressure;
  
  // バッファインデックスを更新
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
  
  // 有効データ数を更新
  if (validDataCount < BUFFER_SIZE) {
    validDataCount++;
  }
}

// 統計データを計算する関数
void calculateStats() {
  if (validDataCount == 0) return; // データがない場合は何もしない
  
  // 平均、最大、最小値の計算のための変数
  float rollSum = 0, pitchSum = 0, vibSum = 0, pressSum = 0;
  float rollMax = -1000, rollMin = 1000;
  float pitchMax = -1000, pitchMin = 1000;
  int vibMax = 0, pressMax = 0, pressMin = 4095;
  int totalVib = 0;
  
  // 合計と最大/最小値の計算
  for (int i = 0; i < validDataCount; i++) {
    // バッファ内の実際のインデックスを計算
    int idx = (bufferIndex - validDataCount + i + BUFFER_SIZE) % BUFFER_SIZE;
    
    // 角度データの集計
    rollSum += rollHistory[idx];
    if (rollHistory[idx] > rollMax) rollMax = rollHistory[idx];
    if (rollHistory[idx] < rollMin) rollMin = rollHistory[idx];
    
    pitchSum += pitchHistory[idx];
    if (pitchHistory[idx] > pitchMax) pitchMax = pitchHistory[idx];
    if (pitchHistory[idx] < pitchMin) pitchMin = pitchHistory[idx];
    
    // 振動データの集計
    vibSum += vibrationsHistory[idx];
    totalVib += vibrationsHistory[idx];
    if (vibrationsHistory[idx] > vibMax) vibMax = vibrationsHistory[idx];
    
    // 圧力データの集計
    pressSum += pressureHistory[idx];
    if (pressureHistory[idx] > pressMax) pressMax = pressureHistory[idx];
    if (pressureHistory[idx] < pressMin && pressureHistory[idx] > 0) pressMin = pressureHistory[idx];
  }
  
  // 平均値の計算
  stats.rollAvg = rollSum / validDataCount;
  stats.pitchAvg = pitchSum / validDataCount;
  stats.vibrationAvg = vibSum / validDataCount;
  stats.pressureAvg = pressSum / validDataCount;
  
  // 最大・最小値の更新
  stats.rollMax = rollMax;
  stats.rollMin = rollMin;
  stats.pitchMax = pitchMax;
  stats.pitchMin = pitchMin;
  stats.vibrationMax = vibMax;
  stats.totalVibrations = totalVib;
  stats.pressureMax = pressMax;
  stats.pressureMin = pressMin;
  
  // 標準偏差の計算
  float rollVarSum = 0, pitchVarSum = 0;
  for (int i = 0; i < validDataCount; i++) {
    int idx = (bufferIndex - validDataCount + i + BUFFER_SIZE) % BUFFER_SIZE;
    rollVarSum += pow(rollHistory[idx] - stats.rollAvg, 2);
    pitchVarSum += pow(pitchHistory[idx] - stats.pitchAvg, 2);
  }
  
  stats.rollStdDev = sqrt(rollVarSum / validDataCount);
  stats.pitchStdDev = sqrt(pitchVarSum / validDataCount);
}

// 統計結果をシリアルに出力する関数
void printStats() {
  Serial.println("\n===== センサーデータ統計 =====");
  
  Serial.println("--- 角度統計 ---");
  Serial.print("Roll (度): 平均=");
  Serial.print(stats.rollAvg, 2);
  Serial.print(", 最大=");
  Serial.print(stats.rollMax, 2);
  Serial.print(", 最小=");
  Serial.print(stats.rollMin, 2);
  Serial.print(", 標準偏差=");
  Serial.println(stats.rollStdDev, 2);
  
  Serial.print("Pitch (度): 平均=");
  Serial.print(stats.pitchAvg, 2);
  Serial.print(", 最大=");
  Serial.print(stats.pitchMax, 2);
  Serial.print(", 最小=");
  Serial.print(stats.pitchMin, 2);
  Serial.print(", 標準偏差=");
  Serial.println(stats.pitchStdDev, 2);
  
  Serial.println("--- 振動統計 ---");
  Serial.print("振動回数 (Hz): 平均=");
  Serial.print(stats.vibrationAvg, 2);
  Serial.print(", 最大=");
  Serial.print(stats.vibrationMax);
  Serial.print(", 累計=");
  Serial.println(stats.totalVibrations);
  
  Serial.println("--- 圧力統計 ---");
  Serial.print("圧力値 (ADC): 平均=");
  Serial.print(stats.pressureAvg, 2);
  Serial.print(", 最大=");
  Serial.print(stats.pressureMax);
  Serial.print(", 最小=");
  Serial.println(stats.pressureMin);
  
  Serial.print("有効データ数: ");
  Serial.println(validDataCount);
  Serial.println("=============================\n");
}

// 統計データをサーバーに送信する関数
void sendStatsToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // 送信用のURLを組み立て
    String serverPath = url; // 実際のエンドポイントに変更する必要があります
    
    // POSTリクエスト用のJSONデータを作成
    String jsonData = "{";
    jsonData += "\"rollAvg\":" + String(stats.rollAvg) + ",";
    jsonData += "\"rollMax\":" + String(stats.rollMax) + ",";
    jsonData += "\"rollMin\":" + String(stats.rollMin) + ",";
    jsonData += "\"rollStdDev\":" + String(stats.rollStdDev) + ",";
    
    jsonData += "\"pitchAvg\":" + String(stats.pitchAvg) + ",";
    jsonData += "\"pitchMax\":" + String(stats.pitchMax) + ",";
    jsonData += "\"pitchMin\":" + String(stats.pitchMin) + ",";
    jsonData += "\"pitchStdDev\":" + String(stats.pitchStdDev) + ",";
    
    jsonData += "\"vibrationAvg\":" + String(stats.vibrationAvg) + ",";
    jsonData += "\"vibrationMax\":" + String(stats.vibrationMax) + ",";
    jsonData += "\"totalVibrations\":" + String(stats.totalVibrations) + ",";
    
    jsonData += "\"pressureAvg\":" + String(stats.pressureAvg) + ",";
    jsonData += "\"pressureMax\":" + String(stats.pressureMax) + ",";
    jsonData += "\"pressureMin\":" + String(stats.pressureMin);
    jsonData += "}";
    
    Serial.println("統計データをサーバーに送信中...");
    Serial.println(jsonData);
    
    // HTTPの実装はコメントアウト - 実際のエンドポイントが必要
    /*
    http.begin(serverPath);
    http.addHeader("Content-Type", "application/json");
    
    int httpResponseCode = http.POST(jsonData);
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP レスポンスコード: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.print("エラーコード: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
    */
  } else {
    Serial.println("Wi-Fiが切断されています");
  }
}

// センサーのキャリブレーション
void calibrateSensor() {
  Serial.println("センサーキャリブレーション中...");
  Serial.println("センサーを水平に保持してください");
  
  // 複数回の測定値の平均を取る
  long sumX = 0, sumY = 0, sumZ = 0;
  const int samples = 100;
  
  for (int i = 0; i < samples; i++) {
    sumX += analogRead(xPin);
    sumY += analogRead(yPin);
    sumZ += analogRead(zPin);
    delay(10);
  }
  
  // 平均値を計算
  int avgX = sumX / samples;
  int avgY = sumY / samples;
  int avgZ = sumZ / samples;
  
  Serial.print("0G ADC値: X=");
  Serial.print(avgX);
  Serial.print(", Y=");
  Serial.print(avgY);
  Serial.print(", Z=");
  Serial.println(avgZ);
  
  Serial.println("キャリブレーション完了");
  delay(500);
}

enum brushState {
  Stop,
  Timer
};

void startRequest() {
  if (WiFi.status() == WL_CONNECTED) { // Wi-Fiが接続されている場合
    HTTPClient http;

    Serial.printf("URLに接続中: %s\n", url);
    http.begin(url); // 指定したURLに接続

    int httpCode = http.GET(); // GETリクエストを送信

    if (httpCode > 0) { // レスポンスが受信できた場合
      Serial.printf("HTTPステータスコード: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) { // ステータスコードが200の場合
        String payload = http.getString(); // レスポンスの内容を取得
        Serial.println("受信したデータ:");
        Serial.println(payload);
      }
    } else {
      Serial.printf("GETリクエストに失敗: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end(); // 接続を終了
  } else {
    Serial.println("Wi-Fiが切断されています。再接続を試みます...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n再接続完了");
  }
}

brushState brushState = Stop;

void loop() {
  ///////////////////////////
  // 加速度センサーの制御 //
  ///////////////////////////
  // 現在時刻を取得
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastTime) / 1000.0; // 秒単位の経過時間

  // 最初の測定または長時間経過していない場合は時間差分の計算をスキップ
  if (isFirstReading || deltaTime > 1.0 || deltaTime <= 0) {
    lastTime = currentTime;
    isFirstReading = false;
    return;
  }
  
  // アナログ値の読み取り
  int xADC = analogRead(xPin);
  int yADC = analogRead(yPin);
  int zADC = analogRead(zPin);
  
  // ADC値を電圧に変換
  float xVoltage = xADC * (vRef / 4095.0);
  float yVoltage = yADC * (vRef / 4095.0);
  float zVoltage = zADC * (vRef / 4095.0);
  
  // 電圧を重力加速度(g)に変換
  float xG = (xVoltage - zeroG) / sensitivity;
  float yG = (yVoltage - zeroG) / sensitivity;
  float zG = (zVoltage - zeroG) / sensitivity;
  
  // ローパスフィルターでノイズを軽減
  filteredAccX = alpha * xG + (1 - alpha) * filteredAccX;
  filteredAccY = alpha * yG + (1 - alpha) * filteredAccY;
  filteredAccZ = alpha * zG + (1 - alpha) * filteredAccZ;
  
  // 加速度から角度を計算（ラジアン）
  // X軸とZ軸の値から計算されるロール角（X-Z平面での傾き）
  float roll = atan2(filteredAccY, filteredAccZ);
  // Y軸とZ軸の値から計算されるピッチ角（Y-Z平面での傾き）
  float pitch = atan2(-filteredAccX, sqrt(filteredAccY * filteredAccY + filteredAccZ * filteredAccZ));
  
  // ラジアンから度に変換
  float rollDeg = roll * 180.0 / PI;
  float pitchDeg = pitch * 180.0 / PI;
  
  // 加速度をm/s²に変換（1g = 9.81 m/s²）
  float accX = filteredAccX * 9.81;
  float accY = filteredAccY * 9.81;
  float accZ = filteredAccZ * 9.81;
  
  // 重力成分の除去（オプション - 静止状態の方がグラフで見やすいので有効化）
  // 静止状態では、Z軸方向に約1Gの重力がかかっています
  // 重力成分を除去することで、実際の動きによる加速度だけを検出します
  float gravityX = -sin(pitch);
  float gravityY = sin(roll) * cos(pitch);
  float gravityZ = cos(roll) * cos(pitch);
  
  // 重力ベクトルの成分を各軸から除去
  accX -= gravityX * 9.81;
  accY -= gravityY * 9.81;
  accZ -= gravityZ * 9.81;
  
  // 加速度の大きさ（合成加速度）を計算
  accelerationMagnitude = sqrt(accX*accX + accY*accY + accZ*accZ);
  
  // 振動回数の検出（ゼロクロッシング法）
  // 加速度が閾値を上回ったか下回ったかを検出することで振動を数える
  if (accelerationMagnitude > VIBRATION_THRESHOLD && !wasAboveThreshold) {
    // 閾値を上回った瞬間をカウント
    vibrationCount++;
    wasAboveThreshold = true;
  } 
  else if (accelerationMagnitude < VIBRATION_THRESHOLD && wasAboveThreshold) {
    // 閾値を下回った状態に戻す
    wasAboveThreshold = false;
  }
  
  // 1秒ごとに振動回数をリセットして出力
  float vibrationsPerSecond = 0;
  
  if (currentTime - lastCountTime >= 1000) { // 1秒経過
    // 1秒あたりの振動回数を計算
    vibrationsPerSecond = vibrationCount;
    
    // デバッグ用（シリアルモニタ表示）
    Serial.print("1秒間の振動回数: ");
    Serial.println(vibrationCount);
    
    // 圧力センサーの読み取り
    int pressureValue = analogRead(volPin);
    
    // データをバッファに追加
    addDataToBuffer(rollDeg, pitchDeg, vibrationCount, pressureValue);
    
    // カウンターをリセット
    vibrationCount = 0;
    lastCountTime = currentTime;
  }
  
  // 現在の値を保存
  lastAccX = accX;
  lastAccY = accY;
  lastAccZ = accZ;
  lastTime = currentTime;
  
  // シリアルプロッタ用のデータ出力
  Serial.print(accelerationMagnitude);
  Serial.print(",");
  Serial.println(vibrationsPerSecond);
  
  //////////////////
  //  加速度終了  //
  //////////////////

  //////////////////////////////
  // 圧力センサとボタンの制御 //
  //////////////////////////////

  // 圧力センサーの値を読み取り
  int sensorValue = analogRead(volPin);
  Serial.print("press value: ");
  Serial.println(sensorValue);

  // ボタンの状態を読み取り
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH && lastButtonState == LOW) {
    if (brushState == Stop) {
      startRequest();
      brushState = Timer;
    }
    
    Serial.println("Button pressed once");
    // ボタンが押されたときに統計データを出力
    calculateStats();
    printStats();
  }
  
  // ボタンの状態によってLEDを制御
  if (buttonState == HIGH) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }
  
  // 前回のボタンの状態を保存
  lastButtonState = buttonState;
  
  //////////////////////////////
  // 統計処理の実行 //
  //////////////////////////////
  
  // 一定間隔で統計データを更新
  if (currentTime - lastStatsUpdateTime >= STATS_UPDATE_INTERVAL) {
    calculateStats();
    lastStatsUpdateTime = currentTime;
    
    // デバッグ用に統計データを表示
    printStats();
  }
  
  // 一定間隔で統計データをサーバーに送信
  if (currentTime - lastStatsSendTime >= STATS_SEND_INTERVAL) {
    sendStatsToServer();
    lastStatsSendTime = currentTime;
  }
  
  delay(50); // メインループの実行間隔
}

/**
 * OpenAIのChat Completions APIを呼び出す関数
 * @param system_role システムロールの内容（変数として差し替え可能）
 * @param user_message ユーザーメッセージの内容
 * @return APIからのレスポンス文字列
 */
String sendChatCompletionRequest(const String& system_role, const String& user_message) {
	HTTPClient http;
	
	// エンドポイントURL
	String endpoint = "https://api.openai.com/v1/chat/completions";
	
	// HTTPクライアントの初期化
	http.begin(endpoint);
	
	// ヘッダーの追加
	http.addHeader("Content-Type", "application/json");
	http.addHeader("Authorization", "Bearer " + String(openai_api_key));
	
	// リクエストボディの作成
	String requestBody = "{";
	requestBody += "\"model\": \"gpt-4o\",";
	requestBody += "\"messages\": [";
	requestBody += "{\"role\": \"system\", \"content\": \"" + system_role + "\"},";
	requestBody += "{\"role\": \"user\", \"content\": \"" + user_message + "\"}";
	requestBody += "]";
	requestBody += "}";
	
	// POSTリクエストの送信
	int httpResponseCode = http.POST(requestBody);
	String response = "";
	
	// レスポンスの処理
	if (httpResponseCode == 200) {
	  response = http.getString();
	  
	  // cJSONを使用してJSONを解析
	  cJSON* root = cJSON_Parse(response.c_str());
	  if (root) {
		cJSON* choices = cJSON_GetObjectItem(root, "choices");
		if (choices && cJSON_IsArray(choices)) {
		  cJSON* firstChoice = cJSON_GetArrayItem(choices, 0);
		  if (firstChoice) {
			cJSON* message = cJSON_GetObjectItem(firstChoice, "message");
			if (message) {
			  cJSON* content = cJSON_GetObjectItem(message, "content");
			  if (content && cJSON_IsString(content)) {
				String result = content->valuestring;
				cJSON_Delete(root);
				http.end();
				return result;
			  }
			}
		  }
		}
		cJSON_Delete(root);
	  } else {
		Serial.println("Failed to parse JSON");
	  }
	} else {
	  Serial.print("HTTP Response code: ");
	  Serial.println(httpResponseCode);
	  response = "HTTPエラー: " + String(httpResponseCode) + " - " + http.getString();
	}
	
	http.end();
	return response;
  }
  
// 使用例
void makeOpenAIChatRequest() {
// 歯科専門家として振る舞うシステムロール
String systemRole = "You are a dental hygiene expert who analyzes toothbrushing technique data. "
					"Your goal is to provide ONE specific, actionable improvement based on sensor data. "
					"Keep your response under 3 sentences and focus on the most important issue. "
					"Be encouraging but direct. Interpret the data as follows: "
					"- Roll angle: ideally between -30 and 30 degrees. Stable is better. "
					"- Pitch angle: ideally around 45 degrees for proper gum line cleaning. "
					"- Vibration: ideal is 150-200 total vibrations per 2-minute session. "
					"- Pressure: optimal range is 150-250 ADC units. Over 300 is too much pressure.";

// センサーデータを含むユーザーメッセージの作成
String userMessage = "Analyze my toothbrushing data"
					"Roll angle (degrees): avg=" + String(stats.rollAvg, 1) + ", max=" + String(stats.rollMax, 1) + 
					", std=" + String(stats.rollStdDev, 1) + ". " +
					"Pitch angle (degrees): avg=" + String(stats.pitchAvg, 1) + ", max=" + String(stats.pitchMax, 1) + 
					", std=" + String(stats.pitchStdDev, 1) + ". " +
					"Vibrations: total=" + String(stats.totalVibrations) + ", avg=" + String(stats.vibrationAvg, 1) + ". " +
					"Pressure (ADC): avg=" + String(stats.pressureAvg, 1) + ", max=" + String(stats.pressureMax) + ". " +
					"What's ONE specific improvement I should make to my brushing technique?";

	// APIリクエストの送信
	String response = sendChatCompletionRequest(systemRole, userMessage);

	// APIリクエストの送信
	String response = sendChatCompletionRequest(systemRole, userMessage);

	// レスポンスの出力
	Serial.println("API Response:");
	Serial.println(response);


}