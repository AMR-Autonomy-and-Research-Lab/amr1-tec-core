// ============================================================
//  OLED TIMER — Feather RP2040
//  Solo usa I2C (Wire) → Pines SDA y SCL
//  Pantalla: SSD1306 128x64 I2C @ 0x3C
//
//  Librerías necesarias (instalar desde el Library Manager):
//    - Adafruit SSD1306
//    - Adafruit GFX Library
//    - (Wire ya viene incluida con el core RP2040)
//
//  NO usa la librería Adafruit CAN — evita el conflicto de
//  arquitectura samd vs rp2040.
// ============================================================

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// --- CONFIGURACIÓN PANTALLA OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1       // pin reset compartido con el MCU
#define SCREEN_ADDRESS 0x3C // dirección I2C típica del SSD1306

// --- LED integrado ---
#define LED_PIN LED_BUILTIN

// --- OBJETO DISPLAY ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- VARIABLES DEL TIMER ---
unsigned long lastTick = 0;
uint32_t timerSeconds = 0;
bool displayOK = false;

// ============================================================
//  Splash — pantalla de arranque con animación
// ============================================================
void showSplash() {
  display.clearDisplay();

  // Doble marco
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawRect(2, 2, 124, 60, SSD1306_WHITE);

  // Título grande
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 8);
  display.print(F("AMR LAB"));

  // Subtítulo
  display.setTextSize(1);
  display.setCursor(28, 32);
  display.print(F("OLED  TIMER"));

  display.display();

  // Barra de carga animada
  for (int i = 0; i <= 100; i += 4) {
    int w = map(i, 0, 100, 0, 104);
    display.fillRect(12, 48, w, 8, SSD1306_WHITE);
    display.display();
    delay(25);
  }
  delay(600);
}

// ============================================================
//  Formatea y dibuja HH:MM:SS en texto grande
// ============================================================
void drawTimer(uint32_t totalSec) {
  uint32_t h = totalSec / 3600;
  uint32_t m = (totalSec % 3600) / 60;
  uint32_t s = totalSec % 60;

  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)h,
           (unsigned long)m, (unsigned long)s);

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 22);
  display.print(buf);
}

// ============================================================
//  Barra de progreso — se llena cada 60 s y se reinicia
// ============================================================
void drawProgressBar(uint32_t totalSec) {
  uint8_t sec60 = totalSec % 60; // 0-59
  int fillW = map(sec60, 0, 59, 0, 118);

  // Marco
  display.drawRoundRect(3, 46, 122, 12, 3, SSD1306_WHITE);
  // Relleno
  if (fillW > 0) {
    display.fillRoundRect(5, 48, fillW, 8, 2, SSD1306_WHITE);
  }

  // Etiqueta debajo (se ve solo si cabe)
  display.setTextSize(1);
  display.setCursor(42, 60);
  display.print(sec60);
  display.print(F(" / 60s"));
}

// ============================================================
//  Heartbeat — circulito que parpadea cada segundo
// ============================================================
void drawHeartbeat(uint32_t totalSec) {
  if (totalSec % 2 == 0) {
    display.fillCircle(121, 5, 3, SSD1306_WHITE);
  } else {
    display.drawCircle(121, 5, 3, SSD1306_WHITE);
  }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // --- Inicializar I2C (usa SDA/SCL del RP2040 por defecto) ---
  Wire.begin();

  // --- Inicializar display OLED ---
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    displayOK = true;
    showSplash();
  } else {
    // Si falla la OLED, parpadear LED rápido como indicador
    displayOK = false;
    while (true) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  }

  lastTick = millis();
}

// ============================================================
//  LOOP — cada 1 segundo actualiza el timer en la OLED
// ============================================================
void loop() {
  unsigned long now = millis();

  if (now - lastTick >= 1000) {
    lastTick += 1000; // evita drift acumulativo
    timerSeconds++;

    // Parpadeo del LED integrado al ritmo del timer
    digitalWrite(LED_PIN, timerSeconds % 2);

    // ---- Dibujar la pantalla ----
    display.clearDisplay();

    // Encabezado
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(4, 2);
    display.print(F("CRONOMETRO"));

    // Línea separadora
    display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

    // Reloj HH:MM:SS grande
    drawTimer(timerSeconds);

    // Barra de progreso (reinicia cada minuto)
    drawProgressBar(timerSeconds);

    // Heartbeat (esquina superior derecha)
    drawHeartbeat(timerSeconds);

    // Enviar al display
    display.display();

    // Debug por Serial
    Serial.print(F("Timer: "));
    Serial.print(timerSeconds);
    Serial.println(F("s"));
  }
}
