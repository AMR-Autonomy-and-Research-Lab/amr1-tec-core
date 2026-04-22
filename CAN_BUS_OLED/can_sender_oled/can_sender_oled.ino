/*
 * ================================================================
 *   FEATHER RP2040 CAN — SENDER CON OLED
 *
 *   Envia mensajes CAN cada ~1 segundo (ID 0x101 y 0x102)
 *   OLED muestra: header AMR, IDs activos, total TX, timer, barra
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

#define CS_PIN    PIN_CAN_CS
#define CAN_BAUDRATE  250000
#define LED_PIN   LED_BUILTIN

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MCP2515 mcp(CS_PIN);

uint32_t      tx_total         = 0;
uint32_t      timerSeconds     = 0;
unsigned long lastScreenUpdate = 0;
bool          displayOK        = false;
bool          justSent         = false;

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
  display.setCursor(18, 28);
  display.print(F("CAN BUS SENDER"));
  display.setCursor(26, 39);
  display.print(F("RP2040  250k"));
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
  display.print(F("AMR1  CAN SENDER"));
  display.setCursor(114, 2);
  display.print(timerSeconds % 2 == 0 ? F("*") : F(" "));
  display.setTextColor(SSD1306_WHITE);

  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  // Bloque 0x101
  if (justSent) {
    display.fillRoundRect(2, 15, 58, 20, 3, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRoundRect(2, 15, 58, 20, 3, SSD1306_WHITE);
  }
  display.setTextSize(1);
  display.setCursor(8, 18);
  display.print(F("ID: 0x101"));
  display.setCursor(10, 27);
  display.print(F("MSG101"));
  display.setTextColor(SSD1306_WHITE);

  // Flecha
  display.setCursor(60, 21);
  display.print(F(">"));

  // Bloque 0x102
  if (justSent) {
    display.fillRoundRect(68, 15, 58, 20, 3, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRoundRect(68, 15, 58, 20, 3, SSD1306_WHITE);
  }
  display.setCursor(74, 18);
  display.print(F("ID: 0x102"));
  display.setCursor(76, 27);
  display.print(F("MSG102"));
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
  display.print(F("TX:#")); display.print(tx_total);

  // Badge CAN
  display.setCursor(0, 58);
  display.print(F("250kbps "));
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
  while (!Serial);

  pinMode(LED_PIN, OUTPUT);

  Wire.begin();
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    displayOK = true;
    showSplash();
  }

  if (!mcp.begin(CAN_BAUDRATE)) {
    Serial.println("Error MCP2515");
    while (1);
  }

  Serial.println("Sender listo");
  lastScreenUpdate = millis();
}

// ================================================================
//  LOOP
// ================================================================
void loop() {
  digitalWrite(LED_PIN, HIGH);

  mcp.beginPacket(0x101);
  mcp.print("MSG101");
  mcp.endPacket();

  mcp.beginPacket(0x102);
  mcp.print("MSG102");
  mcp.endPacket();

  tx_total++;
  justSent = true;
  Serial.println("Mensajes enviados");

  delay(100);
  digitalWrite(LED_PIN, LOW);
  justSent = false;

  // Actualizar OLED cada segundo
  unsigned long now = millis();
  if (now - lastScreenUpdate >= 1000) {
    lastScreenUpdate += 1000;
    timerSeconds++;
    if (displayOK) updateOLED();
  }

  delay(900);
}
