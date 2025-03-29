#include <WiFi.h>
#include <HTTPClient.h>
#include <uTimerLib.h> //Timer: need to install by Library Manager

// set pin numbers

// buttons
const int buttonPin = 14; // ボタンのピン番号
int buttonState = 0;     // 現在のボタンの状態
int lastButtonState = 0; // 前回のボタンの状態

// 加速度センサー
const int xPin = 16; // X軸のピン番号
const int yPin = 17; // Y軸のピン番号
const int zPin = 18; // Z軸のピン番号

const int ledPin = 36;     // LED pin
int led_on = 0;
const int volPin = 2;//圧力センサ用アナログピン
#define PIEZO_SPEAKER_PIN GPIO_NUM_12 //speaker pin


const char* ssid = "hi_guest";
const char* password = "guestP@sS";
const char* url = "http://abehiroshi.la.coocan.jp/top.htm";

#define TIMER_INRERVAL 30000000 // 30秒間隔で実行
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
//////////

void setup() {
  Serial.begin(115200);
  delay(4000); // シリアルモニタの準備待ち

  // ADCの解像度設定（ESP32は12ビットADC）
  analogReadResolution(12); // 必要？

  // initialize the button pin as an input
  pinMode(buttonPin, INPUT);
  // initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);
  // initialize the Pressure Sensor pin as an output
  pinMode(volPin, INPUT);

  // シリアルプロッタ用のラベル設定（合成加速度と振動回数/秒）
  //  Serial.println("AccMagnitude,VibrationsPerSecond");
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

  // Timer setup
  TimerLib.setInterval_us(sendNextSectionRequest, TIMER_INRERVAL);
}

// センサーのキャリブレーション
void calibrateSensor() {
	// カンマなしのメッセージはシリアルプロッタに表示されない
	// このため、代わりにコメント行として出力
	// Serial.println("// センサーキャリブレーション中...");
	// Serial.println("// センサーを水平に保持してください");
	
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
	
	// デバッグメッセージ（プロット時は表示されない）
	// Serial.print("// 0G ADC値: X=");
	// Serial.print(avgX);
	// Serial.print(", Y=");
	// Serial.print(avgY);
	// Serial.print(", Z=");
	// Serial.println(avgZ);
	
	// Serial.println("// キャリブレーション完了");
	delay(500);
	
	// プロッタのヘッダーを再送信
	// Serial.println("AccMagnitude,VibrationsPerSecond");
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
        // Serial.println(payload);
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
  // Serial.print(accelerationMagnitude);
  // Serial.print(",");
  // Serial.println(vibrationsPerSecond);
  
//   delay(50); // サンプリング間隔（50ms = 20Hzサンプリング）
  //////////////////
  //  加速度終了  //
  //////////////////

  //////////////////////////////
  // 圧力センサとボタンの制御 //
  //////////////////////////////

  // read the state of the button value
  int sensorValue = analogRead(volPin);
  Serial.print("press value");
  Serial.println(sensorValue);
  //センサーの値が2000未満になったらLED点灯。そうでなければ消灯。
  //センサーの種類によって感度が変わるので、反応が鈍かったり強かったりした場合は2000を上げたり下げたりして調整してみてください。
  if (sensorValue > 730) {
    Serial.println("=============- generate Alert ===================");
  }

  //button
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH && lastButtonState == LOW) {
    if (brushState == Stop) {
      startRequest();
      brushState = Timer;
    }
    // if (brushState == Timer) {
    //   sectionReset;
    // }
    Serial.println("Button pressed once");
    // 必要な処理を書く
  }
  // Serial.println(buttonState);zzz
  delay(100);
  lastButtonState = buttonState;
}

  //////////////////////////////
  // Timerとbeep //
  //////////////////////////////
void sendNextSectionRequest() {
  Serial.println("send nextSection Request");
  beep();
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(url); // URLを指定
    http.addHeader("Content-Type", "application/json"); // 一応付けておく（ボディなくても）

    Serial.printf("PUTリクエスト送信中: %s\n", url);
    int httpCode = http.sendRequest("PUT"); // ボディなしPUT

    if (httpCode > 0) {
      Serial.printf("HTTPステータスコード: %d\n", httpCode);
      String response = http.getString();
      Serial.println("レスポンス:");
      Serial.println(response);
    } else {
      Serial.printf("PUTリクエストに失敗: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("Wi-Fiが切断されています。再接続中...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n再接続完了");
  }
}

// ビープ音を出す関数, 録音合図のため作成
void beep() {
  const int frequency = 2000; // Hz
  const int resolution = 8; // 8-bit resolution
  const int duty = 2; // 50% デューティ（最大255）

  ledcAttach(PIEZO_SPEAKER_PIN, frequency, resolution);

  ledcWrite(PIEZO_SPEAKER_PIN, duty);  // 音を出す
  delay(1000);                         // 1s
  ledcWrite(PIEZO_SPEAKER_PIN, 0);     // 音を止める
}

