#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>
#include <esp_sntp.h>

// WiFi
const char* ssid = "DUBA 8 FILAJ DIICOT";
const char* password = "dankmemes";

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Web Server
WebServer server(80);

// Stepper
#define DIR_PIN     13
#define STEP_PIN    12
#define SLEEP_PIN   32
#define RESET_PIN   33
#define M0_PIN      14
#define M1_PIN      27
#define M2_PIN      26

// Senzor ultrasonic
#define TRIG_PIN    18
#define ECHO_PIN    19
const float MAX_DISTANCE_CM = 5.6;  // prag rezervor gol

// Setari STEPPER
const int stepsFor90Degrees = 100;
const int stepsFor180Degrees = 200;
const int stepDelayMicros = 6000;

// Control
int doze = 0;
bool directie = true;
int nivelCurent = 0;

// Timp dozare
int ora1 = 8;
int min1 = 0;
int ora2 = 13;
int min2 = 0;
int ora3 = 20;
int min3 = 0;

bool dozatLaOra1 = false;
bool dozatLaOra2 = false;
bool dozatLaOra3 = false;

void setupStepper() {
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(SLEEP_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  pinMode(M2_PIN, OUTPUT);

  digitalWrite(SLEEP_PIN, HIGH);
  digitalWrite(RESET_PIN, HIGH);
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);
  digitalWrite(M2_PIN, LOW);
}

void rotatie(bool sens, int pasi) {
  digitalWrite(DIR_PIN, sens ? HIGH : LOW);
  delay(5);
  for (int i = 0; i < pasi; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(stepDelayMicros);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(stepDelayMicros);
  }
  delay(10);
}

void dozeaza() {
  // Vibrație față-spate înainte de dozare
  const int pasiVibratie = 10;
  const int nrRepetitii = 3;

  for (int i = 0; i < nrRepetitii; i++) {
    rotatie(true, pasiVibratie);   // Față
    delay(50);
    rotatie(false, pasiVibratie);  // Spate
    delay(50);
  }

  // Actiunea de dozare
  rotatie(directie, stepsFor90Degrees);
  doze++;

  // Actualizare pe LCD
  lcd.setCursor(0, 0);
  lcd.print("Doze: ");
  lcd.print(doze);
  lcd.print("     ");
}

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration * 0.034 / 2.0;
}

int calculNivel(float dist) {
  if (dist > MAX_DISTANCE_CM) return 0;
  int nivel = 100 - (dist * 100.0 / MAX_DISTANCE_CM);
  return constrain(nivel, 0, 100);
}

void verificaNivelHrana() {
  float dist = readDistanceCM();
  nivelCurent = calculNivel(dist);
  lcd.setCursor(0, 1);
  lcd.print("Nivel: ");
  lcd.print(nivelCurent);
  lcd.print("%    ");
}

void resetDoze() {
  doze = 0;
}

void syncTime() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("sincronizare");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nsincronizare reusita");
}

String pad2(int val) {
  if (val < 10) return "0" + String(val);
  return String(val);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Dispenser</title>";
  html += "<style>"
          "body { font-family: Arial, sans-serif; margin: 0; padding: 10px; background: #f4f4f4; text-align: center; }"
          "h2 { font-size: 1.8em; margin-bottom: 0.3em; color: #333; }"
          "p { font-size: 1.2em; margin: 0.5em 0; }"
          "form { background: white; padding: 15px; margin: 15px auto; max-width: 400px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.15); }"
          "input[type=number] { font-size: 1.1em; padding: 6px 8px; width: 60px; margin: 6px 5px; border: 1px solid #ccc; border-radius: 4px; }"
          "label { font-size: 1.1em; }"
          "button { font-size: 1.3em; padding: 12px 0; width: 100%; margin-top: 12px; border: none; border-radius: 6px; background-color: #007BFF; color: white; cursor: pointer; }"
          "button:hover { background-color: #0056b3; }"
          "@media (max-width: 450px) {"
          "  form { padding: 10px; margin: 10px 5px; }"
          "  input[type=number] { width: 50px; margin: 5px 3px; }"
          "  h2 { font-size: 1.4em; }"
          "  p { font-size: 1em; }"
          "}"
          "</style></head><body>";

  html += "<h2>Dispenser Web Control</h2>";
  html += "<p><strong>Doze oferite: " + String(doze) + "</strong></p>";
  html += "<p>Nivel curent hrana: <strong>" + String(nivelCurent) + "%</strong></p>";

  html += "<form action='/sethours' method='POST'>";
  html += "<label>Dozare 1: ora</label> <input type='number' name='ora1' min='0' max='23' value='" + String(ora1) + "'>";
  html += "<label>minut</label> <input type='number' name='min1' min='0' max='59' value='" + String(min1) + "'><br>";

  html += "<label>Dozare 2: ora</label> <input type='number' name='ora2' min='0' max='23' value='" + String(ora2) + "'>";
  html += "<label>minut</label> <input type='number' name='min2' min='0' max='59' value='" + String(min2) + "'><br>";

  html += "<label>Dozare 3: ora</label> <input type='number' name='ora3' min='0' max='23' value='" + String(ora3) + "'>";
  html += "<label>minut</label> <input type='number' name='min3' min='0' max='59' value='" + String(min3) + "'><br>";

  html += "<button type='submit'>Seteaza orele</button>";
  html += "</form>";

  html += "<form action='/dozeaza' method='POST'>";
  html += "<button type='submit'>Dozeaza</button>";
  html += "</form>";

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    html += "<p>Ora curenta: " + pad2(timeinfo.tm_hour) + ":" + pad2(timeinfo.tm_min) + ":" + pad2(timeinfo.tm_sec) + "</p>";
  } else {
    html += "<p>Ora curenta: -</p>";
  }

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSetHours() {
  if (server.hasArg("ora1")) ora1 = constrain(server.arg("ora1").toInt(), 0, 23);
  if (server.hasArg("min1")) min1 = constrain(server.arg("min1").toInt(), 0, 59);
  if (server.hasArg("ora2")) ora2 = constrain(server.arg("ora2").toInt(), 0, 23);
  if (server.hasArg("min2")) min2 = constrain(server.arg("min2").toInt(), 0, 59);
  if (server.hasArg("ora3")) ora3 = constrain(server.arg("ora3").toInt(), 0, 23);
  if (server.hasArg("min3")) min3 = constrain(server.arg("min3").toInt(), 0, 59);

  dozatLaOra1 = dozatLaOra2 = dozatLaOra3 = false;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDozeaza() {
  dozeaza();
  server.sendHeader("Location", "/");
  server.send(303);
}

unsigned long tNivel = 0;
unsigned long tReset = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(23, 22);
  lcd.begin(16, 2);
  lcd.backlight();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  setupStepper();

  WiFi.begin(ssid, password);
  Serial.print("Conectare la WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectat");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(20000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Doze: 0");
  lcd.setCursor(0, 1);
  lcd.print("Nivel: ??%");

  syncTime();

  server.on("/", handleRoot);
  server.on("/sethours", HTTP_POST, handleSetHours);
  server.on("/dozeaza", HTTP_POST, handleDozeaza);
  server.begin();
}

void loop() {
  server.handleClient();

  unsigned long cur = millis();

  if (cur - tNivel >= 5000) {
    verificaNivelHrana();
    tNivel = cur;
  }

  if (cur - tReset >= 86400000) {
    resetDoze();
    dozatLaOra1 = dozatLaOra2 = dozatLaOra3 = false;
    tReset = cur;
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int h = timeinfo.tm_hour;
    int m = timeinfo.tm_min;
    int s = timeinfo.tm_sec;

    if (h == ora1 && m == min1 && s == 0 && !dozatLaOra1) {
      dozeaza();
      dozatLaOra1 = true;
    }
    if (h == ora2 && m == min2 && s == 0 && !dozatLaOra2) {
      dozeaza();
      dozatLaOra2 = true;
    }
    if (h == ora3 && m == min3 && s == 0 && !dozatLaOra3) {
      dozeaza();
      dozatLaOra3 = true;
    }
  }
}
