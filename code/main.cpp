#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

const char* ssid = "ESP32-FallDetector";
const char* password = "12345678";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

float gBefore[3], gAfter[3];
float thresholdImpact = 2.5;
float thresholdStill = 1.0;
float angleThreshold = 15.0;
bool fallDetected = false;
unsigned long fallStartTime = 0;

float getAccelMagnitude();
void recordGravity(float g[3]);
float computeRotationAngle(float g1[3], float g2[3]);

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  WiFi.softAP(ssid, password);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();

  delay(2000);
  recordGravity(gBefore);
}

void loop() {
  static unsigned long lastSend = 0;
  static String status = "Normal";

  float accMag = getAccelMagnitude();
  if (!fallDetected && accMag > thresholdImpact) {
    fallDetected = true;
    fallStartTime = millis();
  }

  float angle = 0;

  if (fallDetected && millis() - fallStartTime > 2000) {
    float accMagAfter = getAccelMagnitude();

    if (abs(accMagAfter - thresholdStill) < 0.3) {
      recordGravity(gAfter);
      angle = computeRotationAngle(gBefore, gAfter);

      if (abs(angle - 90.0) < angleThreshold || accMagAfter < 1.05) {
        status = "Fall";
      } else {
        status = "Still";
      }

      fallDetected = false;
      recordGravity(gBefore);
    }
  }


  if (millis() - lastSend > 1000) {
    StaticJsonDocument<200> doc;
    doc["acc"] = accMag;
    doc["angle"] = angle;
    doc["status"] = status;

    String msg;
    serializeJson(doc, msg);
    ws.textAll(msg);

  
    status = "Normal";
    lastSend = millis();
  }
}

float getAccelMagnitude() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float axf = ax / 16384.0;
  float ayf = ay / 16384.0;
  float azf = az / 16384.0;
  return sqrt(axf * axf + ayf * ayf + azf * azf);
}

void recordGravity(float g[3]) {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  g[0] = ax / 16384.0;
  g[1] = ay / 16384.0;
  g[2] = az / 16384.0;
}

float computeRotationAngle(float g1[3], float g2[3]) {
  float dot = g1[0]*g2[0] + g1[1]*g2[1] + g1[2]*g2[2];
  float mag1 = sqrt(g1[0]*g1[0] + g1[1]*g1[1] + g1[2]*g1[2]);
  float mag2 = sqrt(g2[0]*g2[0] + g2[1]*g2[1] + g2[2]*g2[2]);
  float cosTheta = dot / (mag1 * mag2);
  cosTheta = constrain(cosTheta, -1.0, 1.0);
  return acos(cosTheta) * 180.0 / PI;
}
