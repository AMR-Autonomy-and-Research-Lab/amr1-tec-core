/**
 * AMR1 - VEHICLE CONTROL DASHBOARD + OLED
 * Hardware: Adafruit Feather RP2040 CAN
 *
 * Core 1: SBUS decoding (inverted UART, 100kbps 8E2)
 * Core 0: ANSI Serial dashboard + OLED display
 *
 * OLED layout (128x64):
 *   Line 1: Header + failsafe indicator
 *   Line 2: VEL bar + value
 *   Line 3: DIR bar + value
 *   Line 4: BRAKE bar + value
 *   Line 5: [EN] [REV] [GEAR] status
 *
 * Libraries needed:
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *   - Wire (included with RP2040 core)
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "hardware/gpio.h"

// --- SBUS CONFIG ---
#define SBUS_PIN_RX    1
#define BAUD_SBUS      100000
#define SBUS_FRAME_SIZE 25

// --- OLED CONFIG ---
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define SCREEN_ADDRESS  0x3C

// --- OBJECTS ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool displayOK = false;

// --- GLOBAL STATE ---
struct SBUS_State {
  uint16_t ch[16];
  bool failsafe;
  unsigned long last_update;
  int valorVel;
  int valorDir;
  int valorFreno;
  int valorEnable;
  int valorReversa;
  bool highGear;
};

volatile SBUS_State vehicle;

// ================================================================
//  SBUS DECODING
// ================================================================
void parseSBUS(uint8_t *packet) {
  vehicle.ch[0]  = ((packet[1]    | packet[2]  << 8)                        & 0x07FF);
  vehicle.ch[1]  = ((packet[2]>>3 | packet[3]  << 5)                        & 0x07FF);
  vehicle.ch[2]  = ((packet[3]>>6 | packet[4]  << 2 | packet[5]  << 10)     & 0x07FF);
  vehicle.ch[3]  = ((packet[5]>>1 | packet[6]  << 7)                        & 0x07FF);
  vehicle.ch[4]  = ((packet[6]>>4 | packet[7]  << 4)                        & 0x07FF);
  vehicle.ch[5]  = ((packet[7]>>7 | packet[8]  << 1 | packet[9]  << 9)      & 0x07FF);
  vehicle.ch[6]  = ((packet[9]>>2 | packet[10] << 6)                        & 0x07FF);
  vehicle.ch[7]  = ((packet[10]>>5| packet[11] << 3)                        & 0x07FF);
  vehicle.ch[8]  = ((packet[12]   | packet[13] << 8)                        & 0x07FF);
  vehicle.ch[9]  = ((packet[13]>>3| packet[14] << 5)                        & 0x07FF);
  vehicle.ch[10] = ((packet[14]>>6| packet[15] << 2 | packet[16] << 10)     & 0x07FF);
  vehicle.ch[11] = ((packet[16]>>1| packet[17] << 7)                        & 0x07FF);
  vehicle.ch[12] = ((packet[17]>>4| packet[18] << 4)                        & 0x07FF);
  vehicle.ch[13] = ((packet[18]>>7| packet[19] << 1 | packet[20] << 9)      & 0x07FF);
  vehicle.ch[14] = ((packet[20]>>2| packet[21] << 6)                        & 0x07FF);
  vehicle.ch[15] = ((packet[21]>>5| packet[22] << 3)                        & 0x07FF);

  vehicle.failsafe = packet[23] & 0x08;

  // CH0: Throttle & Brake
  if (vehicle.ch[0] < 980) {
    vehicle.valorFreno = map(vehicle.ch[0], 980, 172, 150, 179);
    vehicle.valorVel   = 0;
  } else if (vehicle.ch[0] <= 1000) {
    vehicle.valorVel   = 0;
    vehicle.valorFreno = 150;
  } else {
    vehicle.valorVel   = map(vehicle.ch[0], 1000, 1811, 50, 200);
    vehicle.valorFreno = 150;
  }

  // CH1: Steering
  vehicle.valorDir = map(vehicle.ch[1], 172, 1811, 100, 0);

  // CH2: Gears
  vehicle.highGear = (vehicle.ch[2] > 1000);

  // CH3: Reverse
  vehicle.valorReversa = (vehicle.ch[3] < 980) ? 1 : 0;

  // CH4: Enable / Emergency Brake
  if (vehicle.ch[4] < 980) {
    vehicle.valorFreno = 179;
    vehicle.valorEnable = 0;
  } else {
    vehicle.valorEnable = 1;
  }

  if (!vehicle.failsafe) vehicle.last_update = millis();
}

// ================================================================
//  CORE 1: SBUS RX
// ================================================================
void setup1() {
  Serial1.begin(BAUD_SBUS, SERIAL_8E2);
  gpio_set_inover(SBUS_PIN_RX, 1);
}

void loop1() {
  static uint8_t buf[25];
  static int idx = 0;
  while (Serial1.available()) {
    uint8_t c = Serial1.read();
    if (idx == 0 && c != 0x0F) continue;
    buf[idx++] = c;
    if (idx == 25) {
      if (buf[24] == 0x00) parseSBUS(buf);
      idx = 0;
    }
  }
}

// ================================================================
//  OLED HELPERS
// ================================================================

// Draws a compact bar: label(4ch) [====    ] val
// y = top pixel of the row
void drawOLEDRow(const char *label, int val, int minV, int maxV, int y) {
  const int BAR_X = 30;
  const int BAR_W = 70;
  const int BAR_H = 7;

  display.setTextSize(1);
  display.setCursor(0, y);
  display.print(label);

  // Bar outline
  display.drawRect(BAR_X, y, BAR_W, BAR_H, SSD1306_WHITE);

  // Fill
  int fill = map(val, minV, maxV, 0, BAR_W - 2);
  fill = constrain(fill, 0, BAR_W - 2);
  if (fill > 0) display.fillRect(BAR_X + 1, y + 1, fill, BAR_H - 2, SSD1306_WHITE);

  // Value
  display.setCursor(BAR_X + BAR_W + 2, y);
  display.print(val);
}

void updateOLED() {
  display.clearDisplay();

  bool signalLost = vehicle.failsafe || (millis() - vehicle.last_update > 500);

  // --- HEADER ---
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("AMR1 CTRL"));

  // Signal status top-right
  display.setCursor(72, 0);
  if (signalLost) {
    display.print(F("[NO SIGNAL]"));
  } else {
    display.print(F("[ LIVE ]"));
  }

  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  if (signalLost) {
    display.setTextSize(2);
    display.setCursor(8, 24);
    display.print(F("TX LOST!"));
    display.display();
    return;
  }

  // --- BARS (y starts at 12, each row = 10px) ---
  drawOLEDRow("VEL ", vehicle.valorVel,   0,   200, 12);
  drawOLEDRow("DIR ", vehicle.valorDir,   0,   100, 23);
  drawOLEDRow("BRK ", vehicle.valorFreno, 150, 179, 34);

  // --- STATUS FLAGS ---
  display.drawLine(0, 44, 127, 44, SSD1306_WHITE);
  display.setTextSize(1);

  // [EN] block
  display.setCursor(0, 47);
  if (vehicle.valorEnable) {
    display.fillRect(0, 46, 28, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.print(F(" EN "));
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.drawRect(0, 46, 28, 10, SSD1306_WHITE);
    display.print(F(" EN "));
  }

  // [REV] block
  display.setCursor(32, 47);
  if (vehicle.valorReversa) {
    display.fillRect(32, 46, 34, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.print(F(" REV "));
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.drawRect(32, 46, 34, 10, SSD1306_WHITE);
    display.print(F(" REV "));
  }

  // [GEAR] block
  display.setCursor(70, 47);
  if (vehicle.highGear) {
    display.fillRect(70, 46, 56, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.print(F(" HIGH GR "));
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.drawRect(70, 46, 56, 10, SSD1306_WHITE);
    display.print(F(" LOW GR  "));
  }

  // --- RAW CH VALUES (bottom strip) ---
  display.setCursor(0, 57);
  display.print(F("CH:"));
  display.print(vehicle.ch[0]);
  display.print(F(" "));
  display.print(vehicle.ch[1]);
  display.print(F(" "));
  display.print(vehicle.ch[2]);
  display.print(F(" "));
  display.print(vehicle.ch[3]);

  display.display();
}

// ================================================================
//  SERIAL DASHBOARD HELPERS
// ================================================================
void drawBar(int val, int minV, int maxV, int width) {
  int pos = map(val, minV, maxV, 0, width);
  Serial.print("[");
  for (int i = 0; i < width; i++) Serial.print(i < pos ? "=" : " ");
  Serial.print("]");
}

// ================================================================
//  CORE 0: SETUP + LOOP
// ================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);

  Wire.begin();
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    displayOK = true;

    // Splash
    display.clearDisplay();
    display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(12, 10);
    display.print(F("AMR LAB"));
    display.setTextSize(1);
    display.setCursor(10, 36);
    display.print(F("CONTROL DECODER"));
    display.setCursor(22, 50);
    display.print(F("RP2040 CAN"));
    display.display();
    delay(1500);
  }
}

void loop() {
  static unsigned long lastUI = 0;
  if (millis() - lastUI < 150) return;
  lastUI = millis();

  // --- SERIAL ---
  Serial.print("\033[2J\033[H");
  Serial.println(F("==============================================================="));
  Serial.println(F("     AMR1 - FR-SKY TARANIS Q X7 DASHBOARD (RP2040 CAN)        "));
  Serial.println(F("==============================================================="));

  bool signalLost = vehicle.failsafe || (millis() - vehicle.last_update > 500);

  if (signalLost) {
    Serial.println(F("\n [!!!] SIGNAL LOST - CONNECT TRANSMITTER [!!!]"));
  } else {
    Serial.print(F(" STICKS: CH1:")); Serial.print(vehicle.ch[0]);
    Serial.print(F("  CH2:"));        Serial.print(vehicle.ch[1]);
    Serial.print(F("  CH3:"));        Serial.print(vehicle.ch[2]);
    Serial.print(F("  CH4:"));        Serial.println(vehicle.ch[3]);

    Serial.println(F("\n --- COMPUTED VEHICLE STATE ---"));

    Serial.print(F(" VELOCITY (CH1):  ")); drawBar(vehicle.valorVel,   0,   200, 20);
    Serial.print(F("  Out: ")); Serial.println(vehicle.valorVel);

    Serial.print(F(" STEERING (CH2):  ")); drawBar(vehicle.valorDir,   0,   100, 20);
    Serial.print(F("  Out: ")); Serial.println(vehicle.valorDir);

    Serial.print(F(" BRAKE PRESSURE:  ")); drawBar(vehicle.valorFreno, 150, 179, 20);
    Serial.print(F("  Out: ")); Serial.println(vehicle.valorFreno);

    Serial.println(F("\n --- LOGIC STATUS ---"));
    Serial.print(F(" [ENABLE: "));  Serial.print(vehicle.valorEnable  ? "YES" : "NO ");
    Serial.print(F("]  [REVERSE: ")); Serial.print(vehicle.valorReversa ? "YES" : "NO ");
    Serial.print(F("]  [GEAR: "));    Serial.print(vehicle.highGear     ? "HIGH" : "LOW ");
    Serial.println(F("]"));

    Serial.println(F("\n --- AUXILIARY CHANNELS (Buttons/Switches) ---"));
    for (int i = 4; i < 16; i++) {
      Serial.print(F("CH")); Serial.print(i + 1); Serial.print(F(":"));
      if (i < 9) Serial.print(F(" "));
      Serial.print(vehicle.ch[i]);
      if ((i + 1) % 4 == 0) Serial.println(); else Serial.print(F("\t"));
    }
  }

  Serial.println(F("\n==============================================================="));

  // --- OLED ---
  if (displayOK) updateOLED();
}
