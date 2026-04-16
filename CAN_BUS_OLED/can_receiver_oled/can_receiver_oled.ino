/*
 * ================================================================
 *   FEATHER RP2040 CAN — RECEIVER CON OLED TIMER
 *
 *   ✅ Recibe mensajes CAN por ID (configurable)
 *   ✅ Muestra timer HH:MM:SS en la OLED (SDA/SCL — I2C)
 *   ✅ Muestra los mensajes CAN recibidos en la pantalla
 *   ✅ Barra de progreso + heartbeat
 *
 *   Librerías necesarias:
 *     - Adafruit MCP2515 (Adafruit CAN)
 *     - Adafruit SSD1306
 *     - Adafruit GFX Library
 *     - Wire (incluida con el core RP2040)
 *
 *   Pantalla: SSD1306 128x64 I2C @ 0x3C (pines SDA y SCL)
 * ================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MCP2515.h>

// --- PANTALLA OLED ---
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define SCREEN_ADDRESS  0x3C

// --- CAN ---
#define CAN_BAUDRATE   500000

// 🔧 CONFIGURA AQUÍ EL ID QUE QUIERES ESCUCHAR
#define MY_CAN_ID      0x102

// --- LED ---
#define LED_PIN  LED_BUILTIN

// --- OBJETOS ---
Adafruit_SSD1306  display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MCP2515  mcp(PIN_CAN_CS, &SPI1);

// --- VARIABLES ---
unsigned long lastScreenUpdate = 0;
uint32_t timerSeconds  = 0;
uint32_t rx_total      = 0;
bool     canReady      = false;
bool     displayOK     = false;

// Último mensaje recibido (para mostrarlo en pantalla)
char lastMsg[9]    = "------";   // max 8 bytes CAN + null
uint32_t lastMsgId = 0;

// ================================================================
//  SPLASH — pantalla de arranque
// ================================================================
void showSplash() {
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawRect(2, 2, 124, 60, SSD1306_WHITE);

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 8);
  display.print(F("AMR LAB"));

  display.setTextSize(1);
  display.setCursor(14, 32);
  display.print(F("CAN + OLED TIMER"));

  display.display();

  // Barra de carga
  for (int i = 0; i <= 100; i += 4) {
    int w = map(i, 0, 100, 0, 104);
    display.fillRect(12, 48, w, 8, SSD1306_WHITE);
    display.display();
    delay(20);
  }
  delay(400);
}

// ================================================================
//  Dibuja HH:MM:SS en grande
// ================================================================
void drawTimer(uint32_t totalSec) {
  uint32_t h = totalSec / 3600;
  uint32_t m = (totalSec % 3600) / 60;
  uint32_t s = totalSec % 60;

  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
           (unsigned long)h,
           (unsigned long)m,
           (unsigned long)s);

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 16);
  display.print(buf);
}

// ================================================================
//  Barra de progreso 60s
// ================================================================
void drawProgressBar(uint32_t totalSec) {
  uint8_t sec60 = totalSec % 60;
  int fillW = map(sec60, 0, 59, 0, 118);

  display.drawRoundRect(3, 36, 122, 10, 3, SSD1306_WHITE);
  if (fillW > 0) {
    display.fillRoundRect(5, 38, fillW, 6, 2, SSD1306_WHITE);
  }
}

// ================================================================
//  Heartbeat
// ================================================================
void drawHeartbeat(uint32_t totalSec) {
  if (totalSec % 2 == 0) {
    display.fillCircle(121, 5, 3, SSD1306_WHITE);
  } else {
    display.drawCircle(121, 5, 3, SSD1306_WHITE);
  }
}

// ================================================================
//  Info CAN abajo de la pantalla
// ================================================================
void drawCANInfo() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Línea: último mensaje recibido
  display.setCursor(0, 49);
  display.print(F("RX:0x"));
  display.print(lastMsgId, HEX);
  display.print(F(" "));
  display.print(lastMsg);

  // Línea: total recibidos + estado
  display.setCursor(0, 58);
  display.print(F("Total:"));
  display.print(rx_total);
  display.print(F(" "));
  display.print(canReady ? F("[CAN ON]") : F("[CAN --]"));
}

// ================================================================
//  SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // 1️⃣ Inicializar I2C y OLED (SDA/SCL)
  Wire.begin();
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    displayOK = true;
    showSplash();
  } else {
    displayOK = false;
  }

  // 2️⃣ Inicializar SPI1 para CAN
  SPI1.setRX(8);
  SPI1.setTX(15);
  SPI1.setSCK(14);
  SPI1.begin(false);

  // Activar CAN transceiver
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, LOW);
  pinMode(PIN_CAN_RESET, OUTPUT);
  digitalWrite(PIN_CAN_RESET, HIGH);
  delay(10);

  // 3️⃣ Iniciar MCP2515
  canReady = mcp.begin(CAN_BAUDRATE);

  Serial.println(F("============================"));
  Serial.println(F(" RECEIVER + OLED TIMER"));
  Serial.print(F(" CAN: "));
  Serial.println(canReady ? "OK" : "FALLO");
  Serial.print(F(" OLED: "));
  Serial.println(displayOK ? "OK" : "FALLO");
  Serial.print(F(" Escuchando ID: 0x"));
  Serial.println(MY_CAN_ID, HEX);
  Serial.println(F("============================"));

  lastScreenUpdate = millis();
}

// ================================================================
//  LOOP
// ================================================================
void loop() {
  unsigned long now = millis();

  // --- Revisar mensajes CAN (no bloqueante) ---
  if (canReady) {
    int len = mcp.parsePacket();

    if (len > 0) {
      uint32_t id = mcp.packetId();

      // Filtrar por ID
      if (id == MY_CAN_ID) {
        // Leer datos
        uint8_t i = 0;
        memset(lastMsg, 0, sizeof(lastMsg));
        while (mcp.available() && i < 8) {
          lastMsg[i] = (char)mcp.read();
          i++;
        }
        lastMsgId = id;
        rx_total++;

        // Blink LED
        digitalWrite(LED_PIN, HIGH);

        // Serial debug
        Serial.print(F("[RX 0x"));
        Serial.print(id, HEX);
        Serial.print(F("] "));
        Serial.println(lastMsg);
      }
    }
  }

  // --- Actualizar pantalla cada 1 segundo ---
  if (now - lastScreenUpdate >= 1000) {
    lastScreenUpdate += 1000;
    timerSeconds++;

    digitalWrite(LED_PIN, LOW);  // apagar LED del blink

    if (displayOK) {
      display.clearDisplay();

      // Encabezado
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(4, 2);
      display.print(F("CRONOMETRO"));

      display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

      // Timer HH:MM:SS
      drawTimer(timerSeconds);

      // Barra de progreso
      drawProgressBar(timerSeconds);

      // Info CAN (últimos datos recibidos)
      drawCANInfo();

      // Heartbeat
      drawHeartbeat(timerSeconds);

      display.display();
    }

    // Serial timer
    Serial.print(F("Timer: "));
    Serial.print(timerSeconds);
    Serial.print(F("s | RX total: "));
    Serial.println(rx_total);
  }
}
