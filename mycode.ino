#define BLYNK_TEMPLATE_NAME "Flame and Gas alert system"
#define BLYNK_TEMPLATE_ID "TMPL6VX56kHkt"
#define BLYNK_AUTH_TOKEN "afNiF2kvPLN3rNUZwL0VQoz8yAYPP6Dm"

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- Pins ----------------
#define FLAME_PIN D5
#define GAS_PIN   A0
#define RELAY_PIN D6

// ---------------- WiFi ----------------
char ssid[] = "TP-Link_8360";
char pass[] = "fahimislive";

// ---------------- Objects ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;

// ---------------- State ----------------
bool flameReported = false;
bool gasReported   = false;
bool relayFromApp  = false;
unsigned long startTime;


// ---------------- Gas threshold ----------------
#define GAS_THRESHOLD 500   // adjust after testing


// ---------------- Gas averaging ----------------
int readGas() {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(GAS_PIN);
    delay(5);
  }
  return sum / 10;
}

// ---------------- Blynk relay control ----------------
BLYNK_WRITE(V2) {
  relayFromApp = param.asInt(); // 1 or 0
}

// ---------------- Sensor task ----------------
void readSensors() {
  int flameState = digitalRead(FLAME_PIN); // LOW = flame
  int gasValue   = readGas();

  // ---- Relay logic (safety has priority) ----
  if (flameState == LOW) {
    digitalWrite(RELAY_PIN, LOW);   // force ON
  } else {
    digitalWrite(RELAY_PIN, relayFromApp ? LOW : HIGH);
  }

  // ---- LCD ----
  lcd.setCursor(0, 0);
  lcd.print("GAS: ");
  lcd.setCursor(5, 0);
  lcd.print("     ");
  lcd.setCursor(5, 0);
  lcd.print(gasValue);

  lcd.setCursor(0, 1);
  lcd.print("Status:        ");
  lcd.setCursor(8, 1);
  lcd.print(flameState == LOW ? "FLAME   " : "NO FLAME");

  // ---- Blynk widgets ----
  Blynk.virtualWrite(V0, gasValue);
  Blynk.virtualWrite(V1, flameState == LOW ? 1 : 0);

  // ---- Flame event (edge-triggered) ----
  if (flameState == LOW && !flameReported) {
    Blynk.logEvent("flame_detected", "🔥 Flame detected!");
    flameReported = true;
  }
  if (flameState == HIGH) {
    flameReported = false;
  }

  // ---- Gas threshold event (edge-triggered) ----
// Ignore gas alerts for first 60 seconds
if (millis() - startTime > 60000) {

  if (gasValue > GAS_THRESHOLD && !gasReported) {
    Blynk.logEvent("gas_detected", "⚠️ Gas level exceeded threshold!");
    gasReported = true;
  }

  if (gasValue < GAS_THRESHOLD - 50) {
    gasReported = false;
  }
}

}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(9600);
  startTime = millis();


  pinMode(FLAME_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // relay OFF

  Wire.begin(D2, D1);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(500L, readSensors);
}

// ---------------- Loop ----------------
void loop() {
  Blynk.run();
  timer.run();
}
