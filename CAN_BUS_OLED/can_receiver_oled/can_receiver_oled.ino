/*
 * ================================================================
 *   FEATHER RP2040 CAN — RECEIVER CON OLED
 *
 *   Escucha SOLO un ID especifico (configurable)
 *   OLED muestra: header AMR, ultimo mensaje RX, total, timer, barra
 *
 *   Librerias:
 *     - Adafruit MCP2515
 *     - Adafruit SSD1306
 *     - Adafruit GFX Library
 *     - Wire
 * ================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MCP2515.h>

// --- OLED ---
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define SCREEN_ADDRESS  0x3C

// --- CAN ---
#define CAN_BAUDRATE  500000
#define MY_CAN_ID     0x102

#define LED_PIN  LED_BUILTIN

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MCP2515 mcp(PIN_CAN_CS, &SPI1);

uint32_t      rx_total         = 0;
uint32_t      timerSeconds     = 0;
unsigned long lastScreenUpdate = 0;
bool          displayOK        = false;
bool          justReceived     = false;

char     lastData[9] = "------";
uint32_t lastId      = 0;

// ================================================================
//  SPLASH
// ================================================================
void showSplash() {
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawRect(2, 2, 124, 60, SSD1306_WHITE);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 6);
  display.print(F("AMR LAB"));
  display.setTextSize(1);
  display.setCursor(14, 28);
  display.print(F("CAN BUS RECEIVER"));
  display.setCursor(26, 39);
  display.print(F("RP2040  500k"));
  display.display();
  for (int i = 0; i <= 100; i += 2) {
    display.fillRect(12, 52, map(i, 0, 100, 0, 104), 6, SSD1306_WHITE);
    display.display();
    delay(15);
  }
  delay(400);
}

// ================================================================
//  OLED PRINCIPAL
// ================================================================
void updateOLED() {
  display.clearDisplay();

  // Header invertido
  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print(F("AMR1  CAN RECEIVER"));
  display.setCursor(114, 2);
  display.print(timerSeconds % 2 == 0 ? F("*") : F(" "));
  display.setTextColor(SSD1306_WHITE);

  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  // Bloque ID recibido
  if (justReceived) {
    display.fillRoundRect(2, 15, 124, 20, 3, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRoundRect(2, 15, 124, 20, 3, SSD1306_WHITE);
  }
  display.setTextSize(1);
  display.setCursor(8, 18);
  display.print(F("ID: 0x")); display.print(lastId, HEX);
  display.setCursor(8, 27);
  display.print(F("Data: ")); display.print(lastData);
  display.setTextColor(SSD1306_WHITE);

  // Barra progreso 60s
  display.drawRoundRect(2, 38, 124, 8, 2, SSD1306_WHITE);
  int fillW = map(timerSeconds % 60, 0, 59, 0, 120);
  if (fillW > 0) display.fillRoundRect(4, 40, fillW, 4, 1, SSD1306_WHITE);

  // Timer
  char tbuf[10];
  snprintf(tbuf, sizeof(tbuf), "%02lu:%02lu:%02lu",
           (unsigned long)(timerSeconds / 3600),
           (unsigned long)((timerSeconds % 3600) / 60),
           (unsigned long)(timerSeconds % 60));
  display.setCursor(0, 49);
  display.print(tbuf);
  display.setCursor(68, 49);
  display.print(F("RX:#")); display.print(rx_total);

  // Badge CAN
  display.setCursor(0, 58);
  display.print(F("500kbps "));
  display.fillRoundRect(56, 56, 36, 8, 2, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(59, 58);
  display.print(F("CAN ON"));
  display.setTextColor(SSD1306_WHITE);

  display.display();
}

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  pinMode(LED_PIN, OUTPUT);

  Wire.begin();
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    displayOK = true;
    showSplash();
  }

  SPI1.setRX(8);
  SPI1.setTX(15);
  SPI1.setSCK(14);
  SPI1.begin(false);

  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, LOW);
  pinMode(PIN_CAN_RESET, OUTPUT);
  digitalWrite(PIN_CAN_RESET, HIGH);
  delay(10);

  if (!mcp.begin(CAN_BAUDRATE)) {
    Serial.println("ERROR: MCP25625 no responde");
    while (1);
  }

  Serial.println("============================");
  Serial.println(" RECEIVER POR ID");
  Serial.print(" Escuchando ID: 0x");
  Serial.println(MY_CAN_ID, HEX);
  Serial.println("============================");
  Serial.println("CAN OK");

  lastScreenUpdate = millis();
}

// ================================================================
//  LOOP
// ================================================================
void loop() {
  int len = mcp.parsePacket();

  if (len > 0) {
    uint32_t id = mcp.packetId();

    if (id == MY_CAN_ID) {
      uint8_t i = 0;
      memset(lastData, 0, sizeof(lastData));
      while (mcp.available() && i < 8) {
        lastData[i++] = (char)mcp.read();
      }
      lastId = id;
      rx_total++;
      justReceived = true;

      Serial.print(F("[RX ID 0x"));
      Serial.print(id, HEX);
      Serial.print(F("] Data: "));
      Serial.println(lastData);

      digitalWrite(LED_PIN, HIGH);
      if (displayOK) updateOLED();
      delay(20);
      digitalWrite(LED_PIN, LOW);
      justReceived = false;
    } else {
      while (mcp.available()) mcp.read(); // flush otros IDs
    }
  }

  // Actualizar OLED cada segundo
  unsigned long now = millis();
  if (now - lastScreenUpdate >= 1000) {
    lastScreenUpdate += 1000;
    timerSeconds++;
    if (displayOK) updateOLED();
  }
}
