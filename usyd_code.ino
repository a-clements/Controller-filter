#include <Wire.h>
#include <U8g2lib.h>

// -------- OLED (SH1106 128x64 I2C) --------
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// -------- Pin definition --------
const int gamebuttonPin   = 14;
const int opticouplerPin  = 25;

// Rotary encoder
const int intentbuttonPin = 27;   // SW
const int encoderCLK      = 33;
const int encoderDT       = 32;

const int intentLED       = 12;

// -------- Adjustable interval --------
unsigned long minPressIntervalMs = 2000;  // 50–2000 ms via rotary

// -------- State --------
bool signalProcessingOn = false;
unsigned long lastAcceptedPressMs = 0;

int lastIntentState = HIGH;   // INPUT_PULLUP idle = HIGH
int lastGameState   = LOW;
int lastCLKState    = LOW;

// -------- Tracking states --------
int rawPressState = 0;
int processedPressState = 0;

// -------- Display update flag --------
bool updateDisplay = true;


// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  pinMode(gamebuttonPin, INPUT);

  pinMode(intentbuttonPin, INPUT_PULLUP);
  pinMode(encoderCLK, INPUT);
  pinMode(encoderDT, INPUT);

  pinMode(opticouplerPin, OUTPUT);
  pinMode(intentLED, OUTPUT);

  digitalWrite(intentLED, LOW);
  digitalWrite(opticouplerPin, LOW);

  lastCLKState = digitalRead(encoderCLK);

  Wire.begin(21, 22);   // ESP32 I2C
  u8g2.begin();

  drawDisplay();       // Initial screen
}


// ===================== DISPLAY FUNCTION =====================
void drawDisplay() {

  u8g2.clearBuffer();

  // Title
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 12, "Desired Delay:");

  // Large number
  u8g2.setFont(u8g2_font_logisoso24_tf);
  char buffer[20];
  sprintf(buffer, "%lu ms", minPressIntervalMs);
  u8g2.drawStr(0, 40, buffer);

  // Intent status
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 60, "Intent:");

  if (signalProcessingOn) {
    u8g2.drawStr(50, 60, "ON");
  } else {
    u8g2.drawStr(50, 60, "OFF");
  }

  u8g2.sendBuffer();
}


// ===================== LOOP =====================
void loop() {

  // -------- ROTARY ENCODER --------
  int currentCLKState = digitalRead(encoderCLK);

  if (currentCLKState != lastCLKState) {
    if (digitalRead(encoderDT) != currentCLKState) {
      if (minPressIntervalMs > 50) {
        minPressIntervalMs -= 50;
      }
    } else {
      if (minPressIntervalMs < 2000) {
        minPressIntervalMs += 50;
      }
    }
    updateDisplay = true;
  }

  lastCLKState = currentCLKState;

  // -------- INTENT button --------
  int intentState = digitalRead(intentbuttonPin);

  if (lastIntentState == HIGH && intentState == LOW) {
    signalProcessingOn = !signalProcessingOn;

    digitalWrite(intentLED, signalProcessingOn ? HIGH : LOW);

    updateDisplay = true;
  }

  lastIntentState = intentState;

  // -------- GAME button --------
  int buttonState = digitalRead(gamebuttonPin);

  rawPressState = (buttonState == HIGH) ? 1 : 0;

  if (lastGameState == LOW && buttonState == HIGH) {

    if (!signalProcessingOn) {
      digitalWrite(opticouplerPin, HIGH);
      processedPressState = 1;
    } else {
      unsigned long now = millis();
      if (now - lastAcceptedPressMs >= minPressIntervalMs) {
        lastAcceptedPressMs = now;
        digitalWrite(opticouplerPin, HIGH);
        processedPressState = 1;
      } else {
        digitalWrite(opticouplerPin, LOW);
        processedPressState = 0;
      }
    }
  }

  if (lastGameState == HIGH && buttonState == LOW) {
    digitalWrite(opticouplerPin, LOW);
    processedPressState = 0;
  }

  lastGameState = buttonState;

  // -------- Serial CSV --------
  if (signalProcessingOn) {
    Serial.print(rawPressState);
    Serial.print(",");
    Serial.println(processedPressState);
  }

  // -------- Update OLED only when needed --------
  if (updateDisplay) {
    drawDisplay();
    updateDisplay = false;
  }
}
