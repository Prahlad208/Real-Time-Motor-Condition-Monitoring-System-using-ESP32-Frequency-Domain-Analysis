#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <arduinoFFT.h>
#include <math.h>

// ---------------- PIN DEFINITIONS ----------------
#define ENA 25
#define IN1 26
#define IN2 27
#define CURRENT_SENSOR 34

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- FFT SETTINGS ----------------
#define SAMPLES 256
#define SAMPLING_FREQUENCY 2000

double vReal[SAMPLES];
double vImag[SAMPLES];
ArduinoFFT<double> FFT = ArduinoFFT<double>();

// ---------------- PWM ----------------
int pwmFreq = 5000;
int pwmResolution = 8;
int baseDuty = 230;   // 90%
int currentDuty = 230;

// ---------------- SYSTEM STATES ----------------
double zeroLevel = 0;
bool zeroCaptured = false;

double healthyFrequency = 0;
bool calibrated = false;

unsigned long faultTimer = 0;
int faultStage = 0;
bool faultActive = false;

// ------------------------------------------------
// Measure dominant frequency using FFT
// ------------------------------------------------
double measureFrequency() {

  for (int i = 0; i < SAMPLES; i++) {
    vReal[i] = analogRead(CURRENT_SENSOR);
    vImag[i] = 0;
    delayMicroseconds(1000000 / SAMPLING_FREQUENCY);
  }

  FFT.windowing(vReal, SAMPLES, FFTWindow::Hamming, FFTDirection::Forward);
  FFT.compute(vReal, vImag, SAMPLES, FFTDirection::Forward);
  FFT.complexToMagnitude(vReal, vImag, SAMPLES);

  double peak = 0;
  int index = 0;

  for (int i = 1; i < SAMPLES / 2; i++) {
    if (vReal[i] > peak) {
      peak = vReal[i];
      index = i;
    }
  }

  return (index * SAMPLING_FREQUENCY) / SAMPLES;
}

// ------------------------------------------------
void setup() {

  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  ledcAttach(ENA, pwmFreq, pwmResolution);
  ledcWrite(ENA, baseDuty);

  lcd.init();
  lcd.backlight();
  lcd.print("System Booting");
  delay(2000);
  lcd.clear();
}

// ------------------------------------------------
void loop() {

  // ---------- STEP 1: Capture Zero Level ----------
  if (!zeroCaptured) {

    lcd.setCursor(0,0);
    lcd.print("Capturing Zero");

    long sum = 0;
    for (int i = 0; i < 300; i++) {
      sum += analogRead(CURRENT_SENSOR);
      delay(5);
    }

    zeroLevel = sum / 300;
    zeroCaptured = true;

    lcd.clear();
    lcd.print("Zero Stored");
    delay(2000);
    lcd.clear();
    return;
  }

  int current = analogRead(CURRENT_SENSOR);

  // ---------- STEP 2: Wait For Motor Start ----------
  if (!calibrated) {

    if (abs(current - zeroLevel) > 80) {

      lcd.clear();
      lcd.print("Motor Detected");
      delay(2000);

      lcd.clear();
      lcd.print("Calibrating...");
      delay(2000);

      healthyFrequency = measureFrequency();
      calibrated = true;

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Healthy Freq:");
      lcd.setCursor(0,1);
      lcd.print(healthyFrequency);
      lcd.print(" Hz");

      delay(3000);
      faultTimer = millis();
      lcd.clear();
    }
    else {
      lcd.setCursor(0,0);
      lcd.print("Waiting Motor ");
      lcd.setCursor(0,1);
      lcd.print("Start Motor   ");
    }

    return;
  }

  // ---------- STEP 3: Healthy Operation ----------
  if (!faultActive) {

    lcd.setCursor(0,0);
    lcd.print("System Healthy ");
    lcd.setCursor(0,1);
    lcd.print("F:");
    lcd.print(healthyFrequency);
    lcd.print(" Hz   ");
  }

  // ---------- STEP 4: Introduce Fault Every 10 sec ----------
  if (millis() - faultTimer > 10000) {

    faultStage++;
    faultTimer = millis();
    faultActive = true;

    lcd.clear();

    if (faultStage == 1) {
      lcd.print("Fault:Overload");
    }
    else if (faultStage == 2) {
      lcd.print("Fault:Bearing ");
    }
    else if (faultStage == 3) {
      lcd.print("Fault:Misalign");
    }
    else {
      lcd.print("System Restore");
      faultStage = 0;
      faultActive = false;
      ledcWrite(ENA, baseDuty);
      delay(3000);
      lcd.clear();
      return;
    }
  }

  // ---------- STEP 5: Smooth PWM Modulation ----------
  if (faultActive) {

    double faultFreq;

    if (faultStage == 1) faultFreq = 20;     // Overload analogy
    else if (faultStage == 2) faultFreq = 40; // Bearing analogy
    else faultFreq = 60;                      // Misalignment analogy

    double modulation =
        15 * sin(2 * PI * faultFreq * millis() / 1000.0);

    currentDuty = baseDuty + modulation;

    currentDuty = constrain(currentDuty, 200, 240);

    ledcWrite(ENA, currentDuty);

    lcd.setCursor(0,1);
    lcd.print("F:");
    lcd.print(healthyFrequency + faultFreq);
    lcd.print(" Hz   ");
  }

  delay(200);
}
