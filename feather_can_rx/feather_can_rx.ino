/**
 * ========================================================
 *  Feather CAN RX — Receptor por CAN (Serial → TX → CAN → RX)
 * ========================================================
 *
 *  Flujo: en la otra placa (TX) escribes comandos por Serial (B/D/M);
 *  el TX los envía por CAN; esta placa (RX) los recibe y aplica al motor.
 *
 *  Actuador: Pololu Glideforce High-Speed LD, 12kgf, 6" stroke, 12V
 *  (control por PWM + dirección; driver con FLT y sensor de corriente)
 *
 *  Pinout Feather RP2040 CAN (POLOLU):
 *    PWM  = D6   (velocidad)
 *    DIR  = D9   (dirección)
 *    FLT  = D5   (falla, activo bajo)
 *    CS   = A3   (sensor de corriente)
 *  Pistón (feedback): Pot = A0
 *
 *  Sistema de frenos (protocolo compa): setpoint por 0x200, feedback 0x201, emergencia 0x210.
 *  Lazo cerrado con pot A0 (filtro media móvil, calibración RAW_MIN/RAW_MAX).
 *
 *  IDs recibidos:
 *    0x200 Setpoint   (1 byte) = posición objetivo 5–99; responde 0x201 con posición actual
 *    0x210 Emergencia (1 byte) = 1 → ir a 0%, 0 → modo normal
 *    0x42  Freno      (1 byte) = setpoint (compatibilidad), responde 0x201
 *    0x53  Dirección  (1 byte)
 *    0x54  Motor      (5 bytes)
 *    0x20  Comando    (2 bytes — legacy)
 *    0x30  Vista OLED (1 byte: 1=CAN addr, 2=Pololu) — Serial "1"/"2" o TX
 *  ID enviado: 0x201 = posición actual freno (1 byte, 0–100)
 *
 *  Baudrate : 500 kbps
 *  Librería : Adafruit_MCP2515 (API: parsePacket/read)
 *  Display  : SSD1306 128×64 I2C
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
  #define CS_PIN   19               // Feather RP2040 CAN: usa 19 si no está la placa en IDE
  #define LED_PIN  13
#endif

// ─── Configuración CAN ─────────────────────────────────
#define CAN_BAUDRATE    500000        // 500 kbps

#define CAN_ID_SETPOINT 0x200         // Setpoint posición actuador (como compa)
#define CAN_ID_FEEDBACK 0x201        // Respuesta posición actual (1 byte)
#define CAN_ID_EMERGENCIA 0x210      // Emergencia: 1=ir a 0%, 0=normal
#define CAN_ID_FRENO    0x42         // Compatibilidad: mismo setpoint que 0x200
#define CAN_ID_DIR      0x53
#define CAN_ID_MOT      0x54
#define CAN_ID_CMD      0x20          // legacy: 2 bytes [PWM, DIR]
#define CAN_ID_VIEW     0x30          // Modo vista: 1=CAN addr, 2=Pololu
#define MY_CAN_ADDR     0x01          // Dirección CAN de esta placa (cámbiala si tienes varias)

// ─── Frenos: calibración y control (YRA/Machinime/Peter) ────
#define RAW_MIN        820            // Pot A0: mínimo raw (heurístico)
#define RAW_MAX        890            // Máximo raw; más podría dañar el freno — cuidado
#define BRAKE_TOLERANCIA 1            // ±1% zona muerta
#define BRAKE_PWM_MIN  140            // PWM mínimo al mover (compa 140)
#define BRAKE_PWM_MAX  255            // PWM máximo
// Polaridad: 0 = estándar. Si al subir setpoint el actuador baja, pon 1
#define INVERTIR_DIR_ACTUADOR 0

#define DEBUG_CAN       1             // 1 = imprimir tramas por Serial

// ─── OLED 0.96" SSD1306 I2C ────────────────────────────
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define SCREEN_ADDRESS  0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOk = false;

// ─── Objeto CAN ────────────────────────────────────────
Adafruit_MCP2515 mcp(CS_PIN);
bool canOk = false;

// ─── Pines Pololu / actuador Glideforce (Feather RP2040 CAN) ────
const uint8_t PIN_PWM = 6;          // D6  — PWM velocidad
const uint8_t PIN_DIR = 9;          // D9  — Dirección (extender/retraer)
const uint8_t PIN_FLT = 5;          // D5  — Fault (activo bajo)
const uint8_t PIN_CS  = A3;         // A3  — Sensor de corriente
const uint8_t PIN_POT = A0;         // A0  — Pistón: potenciómetro feedback

// ─── Constantes sensor de corriente ────────────────────
const float ADC_REF        = 3.3f;
const int   ADC_RES        = 1023;
const float CS_SENSITIVITY = 0.020f;   // V/A
const float CS_OFFSET      = 0.050f;   // V offset

// ─── Variables de recepción CAN ─────────────────────────
uint8_t lastFreno    = 0;             // 0x42, byte 0
uint8_t lastDir      = 0;             // 0x53, byte 0
uint8_t lastMotVel   = 0;             // 0x54, byte 0
uint8_t lastMotEn    = 0;             // 0x54, byte 1
uint8_t lastMotRev   = 0;             // 0x54, byte 2
uint8_t lastMotLo    = 0;             // 0x54, byte 3
uint8_t lastMotHi    = 0;             // 0x54, byte 4
unsigned long lastRxMs  = 0;
unsigned long rxCount   = 0;

// ─── Variables de salida motor / freno ───────────────────
int   rxPWM     = 0;
int   rxDIR     = 0;
float currentA  = 0.0f;
bool  fault     = false;
int   potRaw    = 0;                // Pistón: lectura pot A0 (raw)
int   setValueBrake = 0;            // Setpoint posición freno (0–100) desde CAN 0x200/0x42
int   posBrake  = 0;                // Posición actual mapeada 0–100
bool  modoEmergencia = false;       // 0x210: true → objetivo 0%

// ─── Filtro media móvil pot (como compa, N=5) ────
#define POT_FILTER_N    5
int   bufferLecturas[POT_FILTER_N];
long  sumaLecturasPot  = 0;
int   indicePot        = 0;
bool  bufferPotLleno   = false;

// ─── Modo vista OLED: 1=solo dir CAN, 2=Pololu ─
uint8_t displayMode = 1;             // Serial "1"/"2" o CAN 0x30

// ─── Temporización OLED ─────────────────────────────────
#define OLED_INTERVAL_MS  250         // refrescar OLED cada 250 ms
unsigned long lastOledMs = 0;

// ─── Serial: buffer para comando vista ──────────────────
#define SERIAL_VIEW_BUF  8
char    serialViewBuf[SERIAL_VIEW_BUF];
uint8_t serialViewIdx = 0;

// ─── Prototipos ─────────────────────────────────────────
void initOLED();
void initCAN();
void initMotorPins();
void pollCAN();
void processPacket(long id, uint8_t* buf, uint8_t len);
void readCurrentSensor();
void enviarFeedbackCAN();            // Envía posBrake por CAN 0x201
void brakeControl();                // Lazo cerrado freno: SetValue vs Pot, actualiza rxPWM/rxDIR
void driveMotor();
void processSerialView();
void refreshDisplay();               // Refresco cada 250 ms
void drawCurrentVista();             // Dibuja la vista actual YA (sin esperar intervalo)
void printMenuInicio();              // Mini interfaz al cargar
void updateOLED_vista1_CAN();       // 1: solo dirección CAN
void updateOLED_vista2_Pololu();    // 2: datos Pololu
void blinkLed();

// ═════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(2500);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println(F("========================================"));
  Serial.println(F("  Feather CAN RX — Receptor"));
  Serial.println(F("========================================"));
  Serial.print(F("  CS Pin    : ")); Serial.println(CS_PIN);
  Serial.print(F("  Baudrate  : ")); Serial.print(CAN_BAUDRATE / 1000);
  Serial.println(F(" kbps"));
  Serial.println(F("========================================"));
  printMenuInicio();

  initMotorPins();
  initOLED();
  initCAN();

  // Inicializar filtro media móvil del pot y setpoint = posición actual (no mover al arranque)
  for (int i = 0; i < POT_FILTER_N; i++) {
    bufferLecturas[i] = analogRead(PIN_POT);
    sumaLecturasPot += bufferLecturas[i];
  }
  bufferPotLleno = true;
  readCurrentSensor();
  setValueBrake = posBrake;
  enviarFeedbackCAN();
}

// ═════════════════════════════════════════════════════════
//  LOOP — Ligero: solo funciones (lógica fuera del loop)
// ═════════════════════════════════════════════════════════
void loop() {
  pollCAN();
  readCurrentSensor();
  brakeControl();                   // Frenos: setpoint 0x42 vs pot → rxPWM, rxDIR
  driveMotor();
  processSerialView();
  refreshDisplay();
}

// ═════════════════════════════════════════════════════════
//  Inicialización pines motor Pololu
// ═════════════════════════════════════════════════════════
void initMotorPins() {
  pinMode(PIN_PWM, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_FLT, INPUT_PULLUP);
  pinMode(PIN_CS,  INPUT);
  pinMode(PIN_POT, INPUT);           // Pistón: pot feedback (A0)

  analogWrite(PIN_PWM, 0);
  digitalWrite(PIN_DIR, LOW);
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
  display.println(F("AMR  CAN RX"));
  display.drawFastHLine(0, 14, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(4, 18);
  display.println(F("500 kbps  Listo."));
  display.setCursor(4, 30);
  display.println(F("1=CAN  2=Pololu"));
  display.display();
}

// ═════════════════════════════════════════════════════════
//  Inicialización CAN
// ═════════════════════════════════════════════════════════
void initCAN() {
  if (!mcp.begin(CAN_BAUDRATE)) {
    Serial.println(F("[ERROR] No se pudo inicializar MCP2515"));
    canOk = false;
    while (1) {                       // Parpadeo rápido = error
      digitalWrite(LED_PIN, HIGH); delay(100);
      digitalWrite(LED_PIN, LOW);  delay(100);
    }
  }
  canOk = true;
  Serial.println(F("[OK] MCP2515 inicializado correctamente"));
}

// ═════════════════════════════════════════════════════════
//  Leer tramas CAN (poll)
// ═════════════════════════════════════════════════════════
void pollCAN() {
  if (!canOk) return;

  int packetSize = mcp.parsePacket();
  if (packetSize <= 0) return;        // nada disponible

  long id      = mcp.packetId();
  uint8_t len  = (uint8_t)packetSize;
  uint8_t buf[8] = {0};

  for (uint8_t i = 0; i < len && i < 8; i++) {
    if (mcp.available()) buf[i] = (uint8_t)mcp.read();
  }

  lastRxMs = millis();
  rxCount++;
  blinkLed();

  // Debug por Serial
  if (DEBUG_CAN) {
    Serial.print(F("[RX] 0x")); Serial.print((uint16_t)id, HEX);
    Serial.print(F(" len=")); Serial.print(len);
    Serial.print(F(" data:"));
    for (uint8_t i = 0; i < len && i < 8; i++) {
      Serial.print(' '); Serial.print(buf[i]);
    }
    Serial.println();
  }

  processPacket(id, buf, len);
}

// ═════════════════════════════════════════════════════════
//  Procesar paquete según ID
// ═════════════════════════════════════════════════════════
void processPacket(long id, uint8_t* buf, uint8_t len) {

  // 0x200 — Setpoint posición (como compa): 5–99, responde 0x201
  if (id == CAN_ID_SETPOINT && len >= 1 && !modoEmergencia) {
    lastFreno = buf[0];
    setValueBrake = (int)buf[0];
    setValueBrake = constrain(setValueBrake, 5, 99);
    enviarFeedbackCAN();
    if (DEBUG_CAN) {
      Serial.print(F("  -> Setpoint 0x200 Set=")); Serial.print(setValueBrake);
      Serial.print(F(" Pos=")); Serial.println(posBrake);
    }
  }

  // 0x210 — Emergencia: 1 = ir a 0%, 0 = modo normal
  if (id == CAN_ID_EMERGENCIA && len >= 1) {
    if (buf[0] == 1) {
      modoEmergencia = true;
      setValueBrake = 0;
    } else if (buf[0] == 0) {
      modoEmergencia = false;
    }
    if (DEBUG_CAN) Serial.println(modoEmergencia ? F("  -> Emergencia ON (0%)") : F("  -> Emergencia OFF"));
  }

  // 0x42 — Freno (compatibilidad): mismo setpoint, responde 0x201
  if (id == CAN_ID_FRENO && len >= 1) {
    lastFreno = buf[0];
    setValueBrake = constrain((int)buf[0], 0, 100);
    enviarFeedbackCAN();
    if (DEBUG_CAN) {
      Serial.print(F("  -> Freno 0x42 Set=")); Serial.print(setValueBrake);
      Serial.print(F(" Pos=")); Serial.println(posBrake);
    }
  }

  // 0x53 — Dirección (1 byte, ≥50 = B, <50 = A)
  if (id == CAN_ID_DIR && len >= 1) {
    lastDir = buf[0];
    rxDIR = (buf[0] >= 50) ? 1 : 0;
    if (DEBUG_CAN) {
      Serial.print(F("  -> Dir raw=")); Serial.print(buf[0]);
      Serial.print(F(" DIR="));         Serial.println(rxDIR);
    }
  }

  // 0x54 — Motor (5 bytes: vel, en, rev, lo, hi)
  if (id == CAN_ID_MOT && len >= 2) {
    lastMotVel = buf[0];
    lastMotEn  = buf[1];
    if (len >= 3) lastMotRev = buf[2];
    if (len >= 4) lastMotLo  = buf[3];
    if (len >= 5) lastMotHi  = buf[4];
    rxPWM = (int)buf[0];
    if (buf[1] == 0) rxPWM = 0;       // enable=0 → parar
    if (len >= 3) rxDIR = (buf[2] != 0) ? 1 : 0;  // rev → dirección actuador
    if (DEBUG_CAN) {
      Serial.print(F("  -> Mot Vel=")); Serial.print(lastMotVel);
      Serial.print(F(" En="));  Serial.print(lastMotEn);
      Serial.print(F(" Rev=")); Serial.print(lastMotRev);
      Serial.print(F(" Lo="));  Serial.print(lastMotLo);
      Serial.print(F(" Hi="));  Serial.println(lastMotHi);
    }
  }

  // 0x20 — Comando legacy (2 bytes: PWM, DIR)
  if (id == CAN_ID_CMD && len >= 2) {
    rxPWM = (int)buf[0];
    rxDIR = (int)(buf[1] & 1);
    if (DEBUG_CAN) {
      Serial.print(F("  -> Cmd PWM=")); Serial.print(rxPWM);
      Serial.print(F(" DIR="));         Serial.println(rxDIR);
    }
  }

  // 0x30 — Modo vista OLED (1=CAN, 2=Pololu)
  if (id == CAN_ID_VIEW && len >= 1) {
    if (buf[0] == 1 || buf[0] == 2) {
      displayMode = buf[0];
      drawCurrentVista();
      Serial.print(F(">>> [CAN] Vista: "));
      Serial.println(displayMode == 1 ? F("CAN (1)") : F("Pololu (2)"));
    }
  }
}

// ═════════════════════════════════════════════════════════
//  Serial: comando "1" / "2" / "3" para cambiar vista OLED (con o sin Enter)
// ═════════════════════════════════════════════════════════
static void setDisplayModeAndRedraw(uint8_t mode) {
  if (mode == 1 || mode == 2) {
    displayMode = mode;
    drawCurrentVista();
  }
}

static void printVistaCambiada(uint8_t mode) {
  Serial.print(F(">>> Vista: "));
  Serial.println(mode == 1 ? F("CAN (1)") : F("Pololu (2)"));
}

void printMenuInicio() {
  Serial.println(F("--- ¿Qué ver en la OLED? ---"));
  Serial.println(F("  1 = Dirección CAN"));
  Serial.println(F("  2 = Datos Pololu (PWM, DIR, I, FLT)"));
  Serial.println(F("  Escribe 1 o 2 (o por CAN desde TX)."));
  Serial.println(F("-----------------------------"));
}

void processSerialView() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialViewIdx > 0) {
        serialViewBuf[serialViewIdx] = '\0';
        if (serialViewBuf[0] == '1') { setDisplayModeAndRedraw(1); printVistaCambiada(1); }
        else if (serialViewBuf[0] == '2') { setDisplayModeAndRedraw(2); printVistaCambiada(2); }
        serialViewIdx = 0;
      }
    } else if (serialViewIdx < SERIAL_VIEW_BUF - 1) {
      serialViewBuf[serialViewIdx++] = c;
      if (serialViewIdx == 1 && (serialViewBuf[0] == '1' || serialViewBuf[0] == '2')) {
        displayMode = (uint8_t)(serialViewBuf[0] - '0');
        drawCurrentVista();
        printVistaCambiada(displayMode);
        serialViewIdx = 0;
      }
    }
  }
}

// ═════════════════════════════════════════════════════════
//  Pot filtrado y porcentaje (como compa)
// ═════════════════════════════════════════════════════════
int leerPotFiltrado() {
  int nueva = analogRead(PIN_POT);
  sumaLecturasPot -= bufferLecturas[indicePot];
  bufferLecturas[indicePot] = nueva;
  sumaLecturasPot += nueva;
  indicePot++;
  if (indicePot >= POT_FILTER_N) { indicePot = 0; bufferPotLleno = true; }
  return bufferPotLleno ? (int)(sumaLecturasPot / POT_FILTER_N) : (int)(sumaLecturasPot / indicePot);
}

uint8_t lecturaToPorcentaje(int raw) {
  raw = constrain(raw, RAW_MIN, RAW_MAX);
  int pct = map(raw, RAW_MIN, RAW_MAX, 100, 0);   // compa: 100 en MIN, 0 en MAX
  return (uint8_t)constrain(pct, 0, 100);
}

void enviarFeedbackCAN() {
  if (!canOk) return;
  mcp.beginPacket(CAN_ID_FEEDBACK);
  mcp.write((uint8_t)posBrake);
  mcp.endPacket();
}

// ═════════════════════════════════════════════════════════
//  Sensor de corriente Pololu
// ═════════════════════════════════════════════════════════
void readCurrentSensor() {
  fault = (digitalRead(PIN_FLT) == LOW);

  int   adc = analogRead(PIN_CS);
  float vcs = (float)adc * ADC_REF / (float)ADC_RES;
  currentA  = (vcs - CS_OFFSET) / CS_SENSITIVITY;
  if (currentA < 0.0f) currentA = 0.0f;

  potRaw = leerPotFiltrado();
  posBrake = lecturaToPorcentaje(potRaw);
}

// ═════════════════════════════════════════════════════════
//  Control frenos — lazo cerrado (como compa: tolerancia 1, PWM 140–255)
// ═════════════════════════════════════════════════════════
void brakeControl() {
  int objetivo = modoEmergencia ? 0 : setValueBrake;
  int error = objetivo - posBrake;

  if (abs(error) <= BRAKE_TOLERANCIA) {
    rxPWM = 0;
    return;
  }
  // error > 0 → subir posición → DIR HIGH (1); error < 0 → bajar → DIR LOW (0)
  rxDIR = (error > 0) ? 1 : 0;
  rxPWM = map(abs(error), 0, 100, BRAKE_PWM_MIN, BRAKE_PWM_MAX);
  rxPWM = constrain(rxPWM, 0, 255);
}

// ═════════════════════════════════════════════════════════
//  Control motor Pololu (aplica rxPWM y rxDIR a los pines)
// ═════════════════════════════════════════════════════════
void driveMotor() {
  if (fault) {
    analogWrite(PIN_PWM, 0);
  } else {
    analogWrite(PIN_PWM, rxPWM);
#if INVERTIR_DIR_ACTUADOR
    digitalWrite(PIN_DIR, rxDIR ? LOW : HIGH);   // Polaridad invertida (Rojo/Negro)
#else
    digitalWrite(PIN_DIR, rxDIR ? HIGH : LOW);
#endif
  }
}

// ═════════════════════════════════════════════════════════
//  OLED — Dibujar vista actual YA (sin esperar intervalo)
// ═════════════════════════════════════════════════════════
void drawCurrentVista() {
  if (!oledOk) return;
  lastOledMs = millis();
  switch (displayMode) {
    case 1: updateOLED_vista1_CAN();    break;
    case 2: updateOLED_vista2_Pololu(); break;
    default: updateOLED_vista1_CAN();   break;
  }
}

// Refresco periódico (cada 250 ms)
void refreshDisplay() {
  if (!oledOk) return;
  if (millis() - lastOledMs < OLED_INTERVAL_MS) return;
  lastOledMs = millis();
  switch (displayMode) {
    case 1: updateOLED_vista1_CAN();    break;
    case 2: updateOLED_vista2_Pololu(); break;
    default: updateOLED_vista1_CAN();   break;
  }
}

// Vista 1: solo dirección CAN — Interfaz producto AMR
void updateOLED_vista1_CAN() {
  if (!oledOk) return;
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(4, 2);
  display.print(F("AMR CAN RX"));
  display.setCursor(SCREEN_WIDTH - 20, 2);
  display.print(F("V1"));
  display.drawFastHLine(0, 11, SCREEN_WIDTH, SSD1306_WHITE);

  display.setCursor(4, 16);
  display.print(F("Dir CAN"));
  display.setCursor(78, 16);
  display.print(F("0x"));
  display.println(MY_CAN_ADDR, HEX);

  display.drawFastHLine(0, 55, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(4, 57);
  display.print(F("RX #"));
  display.println(rxCount);

  display.display();
}

// Vista 2: datos Pololu — Interfaz producto AMR
void updateOLED_vista2_Pololu() {
  if (!oledOk) return;
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(4, 2);
  display.print(F("AMR  Pololu"));
  display.setCursor(SCREEN_WIDTH - 20, 2);
  display.print(F("V2"));
  display.drawFastHLine(0, 11, SCREEN_WIDTH, SSD1306_WHITE);

  display.setCursor(4, 14);
  display.print(F("PWM D6"));
  display.setCursor(78, 14);
  display.println(rxPWM);

  display.setCursor(4, 24);
  display.print(F("DIR D9"));
  display.setCursor(78, 24);
  display.println(rxDIR ? F("B") : F("A"));

  display.setCursor(4, 34);
  display.print(F("I (A)"));
  display.setCursor(78, 34);
  display.println(currentA, 2);

  display.setCursor(4, 44);
  display.print(F("FLT D5"));
  display.setCursor(78, 44);
  if (fault) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.println(F("FAULT"));
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.println(F("OK"));
  }

  display.drawFastHLine(0, 53, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(4, 55);
  display.print(F("Pot A0:"));
  display.print(potRaw);
  display.print(F(" RX#"));
  display.println(rxCount);

  display.display();
}

// ═════════════════════════════════════════════════════════
//  LED indicador (parpadeo al recibir)
// ═════════════════════════════════════════════════════════
void blinkLed() {
  digitalWrite(LED_PIN, HIGH);
  delay(10);
  digitalWrite(LED_PIN, LOW);
}