#include <HTTPClient.h>
#include <WiFi.h>
#include <cmath>
#include <cJSON.h>
#include <driver/i2s.h>
#include "env.h"

// シリアル通信の速度を設定
#define SERIAL_BAUD_RATE 115200
// speaker 
#define PIEZO_SPEAKER_PIN GPIO_NUM_12

void setup() {
  Serial.begin(115200);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void loop() {}


