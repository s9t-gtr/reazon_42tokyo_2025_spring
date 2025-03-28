o#include <WiFi.h>
#include <HTTPClient.h>

// set pin numbers

// buttons
const int buttonPin = 14; // ボタンのピン番号
int buttonState = 0;     // 現在のボタンの状態
int lastButtonState = 0; // 前回のボタンの状態

const int ledPin = 36;     // LED pin
const int volPin = 2;//圧力センサ用アナログピン

const char* ssid = "42Tokyo_Guest";
const char* password = "42-tokyo";
const char* url = "http://abehiroshi.la.coocan.jp/top.htm";


// variable for storing the button status

void setup() {
  Serial.begin(115200);
  // initialize the button pin as an input
  pinMode(buttonPin, INPUT);
  // initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);
  // initialize the Pressure Sensor pin as an output
  pinMode(volPin, INPUT);

  delay(4000); // シリアルモニタの準備待ち

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
  // read the state of the button value
  int sensorValue = analogRead(volPin);
  // Serial.println(sensorValue);
 
  //センサーの値が2000未満になったらLED点灯。そうでなければ消灯。
  //センサーの種類によって感度が変わるので、反応が鈍かったり強かったりした場合は2000を上げたり下げたりして調整してみてください。
  // if (sensorValue < 2000) {
  //   digitalWrite(ledPin, HIGH);
  // }
  // else {
  //   digitalWrite(ledPin, LOW);
  // }


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
  // if the button is pressed
  if (buttonState == HIGH) {
    // turn LED on
    digitalWrite(ledPin, HIGH);

  } else {
    // turn LED off
    digitalWrite(ledPin, LOW);
  }
  lastButtonState = buttonState;
}

