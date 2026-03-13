/**
 * ========================================================
 *  Feather CAN TX — Control manual por Serial → CAN
 * ========================================================
 *
 *  Modo: tú envías comandos por Serial; la placa los reenvía por CAN.
 *  (Como lo hacía Yoni: mensaje a la placa, de ahí por CAN a las demás.)
 *
 *  Comandos por Serial (115200 baud):
 *    B <0-100>     Setpoint actuador freno → CAN 0x200 (como compa)
 *    E 0 / E 1    Emergencia → CAN 0x210 (1=ir a 0%, 0=normal)
 *    D <0-100>     Dir     → CAN 0x53 (1 byte)
 *    M v e r l h   Motor   → CAN 0x54 (5 bytes)
 *    ? o help     Ayuda
 *
 *  Ejemplo: B 50   → posición freno 50% (0x200)
 *           200 85 o 200_85 → formato YRA/Machinime: ID + porcentaje (envía CAN 0x200 con 85)
 *           E 1    → emergencia (actuador a 0%); E 0 → normal
 *
 *  Baudrate CAN : 500 kbps
 *  Librería     : Adafruit_MCP2515
 */

// ─── Librerías ──────────────────────────────────────────
#include <Adafruit_MCP2515.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── Pines según placa ─────────────────────────────────
#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_CAN
  #define CS_PIN   PIN_CAN_CS        // Pin 19 en Feather RP2040 CAN
  #define LED_PIN  13
#else
  #define CS_PIN   19                // Si usas Feather RP2040 CAN con otra placa en IDE, usa 19
  #define LED_PIN  13
#endif

// ─── Configuración CAN ─────────────────────────────────
#define CAN_BAUDRATE    500000        // 500 kbps

#define CAN_ID_SETPOINT 0x200         // Setpoint posición actuador (como compa)
#define CAN_ID_EMERGENCIA 0x210      // Emergencia: 1=0%, 0=normal
#define CAN_ID_DIR      0x53
#define CAN_ID_MOT      0x54
#define CAN_ID_VIEW     0x30          // Vista OLED RX: 1=CAN addr, 2=Pololu

// ─── OLED 0.96" SSD1306 I2C ────────────────────────────
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define SCREEN_ADDRESS  0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOk = false;

// ─── Serial: buffer para comandos ───────────────────────
#define SERIAL_BUF_SIZE  64
char    serialBuf[SERIAL_BUF_SIZE];
uint8_t serialIdx = 0;
unsigned long txCount = 0;

// ─── Objeto CAN ────────────────────────────────────────
Adafruit_MCP2515 mcp(CS_PIN);

// ─── Valores actuales (los que envías por Serial → CAN) ─
uint8_t brakeVal  = 0;
uint8_t dirVal    = 50;
uint8_t motorVel  = 0;
uint8_t motorEn   = 1;
uint8_t motorRev  = 0;
uint8_t motorLo   = 0;
uint8_t motorHi   = 0;

// ─── Prototipos ─────────────────────────────────────────
void initOLED();
void initCAN();
void processSerial();
void parseAndSendCommand(char* line);
/** Formato "ID Porcentaje" o "ID_Porcentaje" (ej. 200 85, 200_85) → envía CAN id con 1 byte */
bool trySendIdPos(char* line);
void printHelp();
void printMenuInicio();   // Mini interfaz: qué ver y opciones
void sendBrake(uint8_t val);         // 0x200 setpoint 0–100
void sendEmergencia(uint8_t on);     // 0x210: 1=emergencia, 0=normal
void sendDir(uint8_t val);
void sendMotor(uint8_t v, uint8_t e, uint8_t r, uint8_t lo, uint8_t hi);
void sendViewMode(uint8_t mode);
void refreshDisplay();   // Temporización + updateOLED (fuera del loop)
void updateOLED();
void blinkLed();

// ═════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println(F(""));
  Serial.println(F("  +----------------------------------+"));
  Serial.println(F("  |  AMR  ·  Feather CAN TX          |"));
  Serial.println(F("  |  Serial->CAN  500 kbps           |"));
  Serial.println(F("  +----------------------------------+"));
  Serial.print(F("  CS Pin: ")); Serial.print(CS_PIN);
  Serial.print(F("   CAN: ")); Serial.print(CAN_BAUDRATE / 1000);
  Serial.println(F(" kbps  [OK]"));
  Serial.println(F(""));
  printMenuInicio();
  printHelp();

  initOLED();
  initCAN();
}

// ═════════════════════════════════════════════════════════
//  LOOP — Ligero: solo funciones (lógica fuera del loop)
// ═════════════════════════════════════════════════════════
void loop() {
  processSerial();
  refreshDisplay();
}

// ═════════════════════════════════════════════════════════
//  Inicialización OLED
// ═════════════════════════════════════════════════════════
void initOLED() {
  Serial.print(F("OLED init... "));
  oledOk = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  if (!oledOk) {
    Serial.println(F("no encontrada"));
    return;
  }
  Serial.println(F("OK"));

  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 4);
  display.println(F("AMR  CAN TX"));
  display.drawFastHLine(0, 14, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(4, 18);
  display.println(F("500 kbps  Serial->CAN"));
  display.setCursor(4, 28);
  display.println(F("B/D/M  1,2=vista RX"));
  display.setCursor(4, 42);
  display.println(F("Listo."));
  display.display();
}

// ═════════════════════════════════════════════════════════
//  Inicialización CAN
// ═════════════════════════════════════════════════════════
void initCAN() {
  Serial.print(F("CAN init (CS="));
  Serial.print(CS_PIN);
  Serial.print(F(", "));
  Serial.print(CAN_BAUDRATE / 1000);
  Serial.println(F(" kbps)..."));
  delay(100);   // Dar tiempo al MCP2515 tras el encendido

  if (!mcp.begin(CAN_BAUDRATE)) {
    Serial.println(F("[ERROR] No se pudo inicializar MCP2515"));
    Serial.println(F("  Revisa: 1) Placa = Adafruit Feather RP2040 CAN"));
    Serial.println(F("  2) Cable/terminal CAN  3) Alimentacion"));
    while (1) {                       // Parpadeo rápido = error
      digitalWrite(LED_PIN, HIGH); delay(100);
      digitalWrite(LED_PIN, LOW);  delay(100);
    }
  }
  Serial.println(F("[OK] MCP2515 inicializado correctamente"));
}

// ═════════════════════════════════════════════════════════
//  Lectura de Serial y envío por CAN
// ═════════════════════════════════════════════════════════
void processSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialIdx > 0) {
        serialBuf[serialIdx] = '\0';
        parseAndSendCommand(serialBuf);
        serialIdx = 0;
      }
    } else if (serialIdx < SERIAL_BUF_SIZE - 1) {
      serialBuf[serialIdx++] = c;
    }
  }
}

// Avanzar 'line' tras espacios y un número (ej. " 128" o " 50")
static void skipSpacesAndNumber(char*& line) {
  while (*line == ' ' || *line == '\t') line++;
  while (*line >= '0' && *line <= '9') line++;
}
// Avanzar tras 5 números (para M v e r lo hi)
static void skipSpacesAndFiveNumbers(char*& line) {
  while (*line == ' ' || *line == '\t') line++;
  for (int i = 0; i < 5 && *line; i++) {
    while (*line >= '0' && *line <= '9') line++;
    while (*line == ' ' || *line == '\t') line++;
  }
}

/** Si la línea es "ID Porcentaje" o "ID_Porcentaje" (ej. 200 85), envía por CAN y devuelve true. */
bool trySendIdPos(char* line) {
  char* p = line;
  while (*p == ' ' || *p == '\t') p++;
  if (!*p || (*p != '-' && (*p < '0' || *p > '9'))) return false;
  long id = strtol(p, &p, 0);
  while (*p == ' ' || *p == '\t' || *p == '_') p++;
  if (!*p || (*p != '-' && (*p < '0' || *p > '9'))) return false;
  long pos = strtol(p, &p, 0);
  if (id < 0 || id > 0x7FF) return false;
  if (pos < 0 || pos > 255) return false;

  mcp.beginPacket((uint16_t)id);
  mcp.write((uint8_t)pos);
  mcp.endPacket();
  txCount++;
  blinkLed();
  if (id == CAN_ID_SETPOINT) brakeVal = (uint8_t)pos;
  Serial.print(F("[TX] ID ")); Serial.print((uint16_t)id);
  Serial.print(F(" → ")); Serial.println((int)pos);
  return true;
}

void parseAndSendCommand(char* line) {
  while (*line == ' ' || *line == '\t') line++;
  if (*line >= '0' && *line <= '9' && trySendIdPos(line)) return;

  while (1) {
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '\0') return;

    char cmd = (char)toupper((int)*line);
    line++;

    if (cmd == '?' || cmd == 'H') {
      if (cmd == 'H') {
        while (*line == ' ') line++;
        if ((line[0] != 'e' && line[0] != 'E') || (line[1] != 'l' && line[1] != 'L') || (line[2] != 'p' && line[2] != 'P')) {
          Serial.println(F("Comando? B / E / D / M / 1 / 2  (? = ayuda)"));
          return;
        }
      }
      printHelp();
      return;
    }

    if (cmd == '1') {
      sendViewMode(1);
      while (*line == ' ' || *line == '\t') line++;
      continue;
    }
    if (cmd == '2') {
      sendViewMode(2);
      while (*line == ' ' || *line == '\t') line++;
      continue;
    }
    if (cmd == 'V') {
      int m = atoi(line);
      if (m == 1 || m == 2) sendViewMode((uint8_t)m);
      else Serial.println(F("V 1=CAN  V 2=Pololu"));
      skipSpacesAndNumber(line);
      continue;
    }

    if (cmd == 'B') {
      int v = atoi(line);
      if (v < 0) v = 0;
      if (v > 100) v = 100;   // Setpoint actuador 0–100 % (0x200)
      brakeVal = (uint8_t)v;
      sendBrake(brakeVal);
      skipSpacesAndNumber(line);
      continue;
    }
    if (cmd == 'E') {
      int v = atoi(line);
      if (v == 1 || v == 0) sendEmergencia((uint8_t)v);
      else Serial.println(F("E 0=normal  E 1=emergencia (0%)"));
      skipSpacesAndNumber(line);
      continue;
    }
    if (cmd == 'D') {
      int v = atoi(line);
      if (v < 0) v = 0;
      if (v > 100) v = 100;
      dirVal = (uint8_t)v;
      sendDir(dirVal);
      skipSpacesAndNumber(line);
      continue;
    }
    if (cmd == 'M') {
      int v, e, r, lo, hi;
      if (sscanf(line, "%d %d %d %d %d", &v, &e, &r, &lo, &hi) >= 2) {
        motorVel = (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
        motorEn  = (uint8_t)(e ? 1 : 0);
        motorRev = (uint8_t)(r ? 1 : 0);
        motorLo  = (uint8_t)(lo < 0 ? 0 : (lo > 255 ? 255 : lo));
        motorHi  = (uint8_t)(hi < 0 ? 0 : (hi > 255 ? 255 : hi));
        sendMotor(motorVel, motorEn, motorRev, motorLo, motorHi);
        skipSpacesAndFiveNumbers(line);
        continue;   // seguir con el siguiente comando (ej. "2")
      } else {
        Serial.println(F("M v e r lo hi (ej: M 100 1 0 0 0)"));
      }
      return;
    }

    Serial.println(F("Comando? B / E / D / M / 1 / 2  (? = ayuda)"));
    return;
  }
}

void printMenuInicio() {
  Serial.println(F("--- ¿Qué ver en la OLED del RX? ---"));
  Serial.println(F("  1 = Dirección CAN"));
  Serial.println(F("  2 = Datos Pololu (PWM, DIR, I, FLT)"));
  Serial.println(F("  Escribe 1 o 2 (en cualquier momento para cambiar). ? = ayuda"));
  Serial.println(F("-----------------------------------"));
}

void printHelp() {
  Serial.println(F("--- Comandos (Serial → CAN) ---"));
  Serial.println(F("  ID Porcentaje  Formato YRA: 200 85 o 200_85 → CAN ID con pos (0-255)"));
  Serial.println(F("  B <0-100>      Setpoint actuador → 0x200 (ej: B 50)"));
  Serial.println(F("  E 0 / E 1     Emergencia → 0x210 (1=0%, 0=normal)"));
  Serial.println(F("  D <0-100>     Dir     → 0x53"));
  Serial.println(F("  M v e r lo hi Motor   → 0x54"));
  Serial.println(F("  1 / 2         Vista OLED RX: 1=CAN  2=Pololu"));
  Serial.println(F("  V 1 / V 2     Igual (vista)"));
  Serial.println(F("  ? / help      Ayuda"));
}

/** 0x200 — Setpoint posición actuador (como compa, 1 byte 0–100) */
void sendBrake(uint8_t val) {
  mcp.beginPacket(CAN_ID_SETPOINT);
  mcp.write(val);
  mcp.endPacket();
  txCount++;
  blinkLed();
  Serial.print(F("[TX] 0x200 Setpoint: "));
  Serial.println(val);
}

/** 0x210 — Emergencia: 1 = ir a 0%, 0 = modo normal */
void sendEmergencia(uint8_t on) {
  mcp.beginPacket(CAN_ID_EMERGENCIA);
  mcp.write(on ? 1 : 0);
  mcp.endPacket();
  txCount++;
  blinkLed();
  Serial.println(on ? F("[TX] 0x210 Emergencia ON (0%)") : F("[TX] 0x210 Emergencia OFF"));
}

/** 0x53 — Dirección (1 byte) */
void sendDir(uint8_t val) {
  mcp.beginPacket(CAN_ID_DIR);
  mcp.write(val);
  mcp.endPacket();
  txCount++;
  blinkLed();
  Serial.print(F("[TX] 0x53 Dir: "));
  Serial.println(val);
}

/** 0x54 — Motor (5 bytes) */
void sendMotor(uint8_t v, uint8_t e, uint8_t r, uint8_t lo, uint8_t hi) {
  mcp.beginPacket(CAN_ID_MOT);
  mcp.write(v);
  mcp.write(e);
  mcp.write(r);
  mcp.write(lo);
  mcp.write(hi);
  mcp.endPacket();
  txCount++;
  blinkLed();
  Serial.print(F("[TX] 0x54 Mot: "));
  Serial.print(v); Serial.print(' ');
  Serial.print(e); Serial.print(' ');
  Serial.print(r); Serial.print(' ');
  Serial.print(lo); Serial.print(' ');
  Serial.println(hi);
}

/** 0x30 — Modo vista OLED en RX (1 byte: 1=CAN, 2=Pololu) */
void sendViewMode(uint8_t mode) {
  if (mode != 1 && mode != 2) mode = 1;
  mcp.beginPacket(CAN_ID_VIEW);
  mcp.write(mode);
  mcp.endPacket();
  txCount++;
  blinkLed();
  Serial.print(F("[TX] 0x30 >>> Vista RX: "));
  Serial.println(mode == 1 ? F("CAN (1)") : F("Pololu (2)"));
}

// ═════════════════════════════════════════════════════════
//  Refresco OLED (cada 250 ms, función fuera del loop)
// ═════════════════════════════════════════════════════════
void refreshDisplay() {
  static unsigned long lastOledMs = 0;
  if (millis() - lastOledMs < 250) return;
  lastOledMs = millis();
  updateOLED();
}

// ═════════════════════════════════════════════════════════
//  Actualizar pantalla OLED — Interfaz producto AMR
// ═════════════════════════════════════════════════════════
void updateOLED() {
  if (!oledOk) return;

  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Cabecera
  display.setCursor(4, 2);
  display.print(F("AMR CAN TX"));
  display.setCursor(SCREEN_WIDTH - 24, 2);
  display.print(F("500k"));
  display.drawFastHLine(0, 11, SCREEN_WIDTH, SSD1306_WHITE);

  // Freno · Dir · Vel (etiquetas alineadas, valores a la derecha)
  display.setCursor(4, 14);
  display.print(F("Freno 0x200"));
  display.setCursor(78, 14);
  display.println(brakeVal);

  display.setCursor(4, 24);
  display.print(F("Dir   0x53"));
  display.setCursor(78, 24);
  display.println(dirVal);

  display.setCursor(4, 34);
  display.print(F("Vel   0x54"));
  display.setCursor(78, 34);
  display.println(motorVel);

  // Motor: En Rev Lo Hi (una línea)
  display.setCursor(4, 44);
  display.print(F("En:"));
  display.print(motorEn);
  display.print(F(" Rev:"));
  display.print(motorRev);
  display.print(F(" Lo:"));
  display.print(motorLo);
  display.print(F(" Hi:"));
  display.println(motorHi);

  // Pie: contador TX
  display.drawFastHLine(0, 55, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(4, 57);
  display.print(F("TX #"));
  display.println(txCount);

  display.display();
}

// ═════════════════════════════════════════════════════════
//  LED indicador
// ═════════════════════════════════════════════════════════
void blinkLed() {
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
}