#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "secrets.h"

// -------- SMTP Configuration --------
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

// -------- Pins --------
#define MAX4466_PIN 34
#define BUTTON_PIN 18

// -------- Thresholds --------
#define VOICE_THRESHOLD 2500
#define VOICE_DURATION 200
#define HEART_RATE_MIN 50
#define HEART_RATE_MAX 120
#define COOLDOWN_PERIOD 10000

// -------- Objects --------
MAX30105 heartSensor;
WiFiClientSecure client;

// -------- Variables --------
unsigned long lastAlertTime = 0;
bool alertInProgress = false;
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

// -------- Gmail retry control --------
bool gmailPending = false;
String gmailSubject = "";
String gmailBody = "";
int gmailRetryCount = 0;
#define MAX_RETRIES 3

// -------- Base64 Encoding --------
const char* b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
String base64Encode(String input) {
  String output = "";
  int val = 0, valb = -6;
  for (int i = 0; i < input.length(); i++) {
    val = (val << 8) + (uint8_t)input[i];
    valb += 8;
    while (valb >= 0) {
      output += b64chars[(val >> valb) & 0x3F];
      valb -= 6;
    }
  }
  if (valb > -6) output += b64chars[((val << 8) >> (valb + 8)) & 0x3F];
  while (output.length() % 4) output += '=';
  return output;
}

// -------- WiFi --------
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi Connected");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi Connection Failed");
  }
}

// -------- Voice Detection --------
bool checkVoiceActivation() {
  int level = analogRead(MAX4466_PIN);
  if (level > VOICE_THRESHOLD) {
    delay(VOICE_DURATION);
    return analogRead(MAX4466_PIN) > VOICE_THRESHOLD;
  }
  return false;
}

// -------- Heart Rate --------
int readHeartRate() {
  long irValue = heartSensor.getIR();
  if (irValue < 50000) return 0;

  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute > 20 && beatsPerMinute < 255) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;
      for (byte i = 0; i < RATE_SIZE; i++) beatAvg += rates[i];
      beatAvg /= RATE_SIZE;

      Serial.print("BPM: ");
      Serial.println(beatAvg);
      return beatAvg;
    }
  }
  return 0;
}

// -------- SOS Trigger --------
void triggerSOS(String reason) {
  if (millis() - lastAlertTime < COOLDOWN_PERIOD) return;

  lastAlertTime = millis();
  Serial.println("🚨 SOS Triggered: " + reason);

  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    gmailSubject = "🚨 EMERGENCY SOS ALERT";
    gmailBody = "Emergency detected: " + reason;
    gmailPending = true;
  }
}

// -------- Send Email --------
bool sendGmail(String subject, String body) {
  client.setInsecure();

  if (!client.connect(SMTP_HOST, SMTP_PORT)) return false;

  client.println("EHLO esp32");
  delay(150);
  client.println("AUTH LOGIN");
  delay(150);
  client.println(base64Encode(AUTHOR_EMAIL));
  delay(150);
  client.println(base64Encode(AUTHOR_PASSWORD));
  delay(200);

  client.println("MAIL FROM:<" + String(AUTHOR_EMAIL) + ">");
  delay(150);
  client.println("RCPT TO:<" + String(RECIPIENT_EMAIL) + ">");
  delay(150);
  client.println("DATA");
  delay(150);

  client.println("Subject: " + subject);
  client.println("Content-Type: text/plain");
  client.println();
  client.println(body);
  client.println(".");
  client.println("QUIT");

  client.stop();
  return true;
}

// -------- Setup --------
void setup() {
  Serial.begin(115200);

  pinMode(MAX4466_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(21, 22);

  if (heartSensor.begin(Wire)) {
    heartSensor.setup();
    heartSensor.setPulseAmplitudeRed(0x0A);
  }

  connectWiFi();
}

// -------- Loop --------
void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    triggerSOS("Manual Button Press");
    delay(500);
  }

  if (checkVoiceActivation()) {
    triggerSOS("Voice Distress Detected");
  }

  int bpm = readHeartRate();
  if (bpm > 0 && (bpm < HEART_RATE_MIN || bpm > HEART_RATE_MAX)) {
    triggerSOS("Abnormal Heart Rate: " + String(bpm));
  }

  if (gmailPending) {
    if (sendGmail(gmailSubject, gmailBody)) {
      gmailPending = false;
    }
  }

  delay(100);
}