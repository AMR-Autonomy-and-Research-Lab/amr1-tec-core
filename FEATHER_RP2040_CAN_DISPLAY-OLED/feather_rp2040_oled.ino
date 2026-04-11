#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Definiciones de la pantalla OLED
#define SCREEN_WIDTH 128 // Ancho de la pantalla en pixeles
#define SCREEN_HEIGHT 64 // Alto de la pantalla en pixeles (ajusta a 32 si usas una ms pequea)

// Reset pin no es usado en la mayora de los mdulos I2C modernos
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C // Direccin I2C comn: 0x3C o 0x3D

// Inicializa el objeto para la pantalla
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- NOTA PARA SH1106 ---
// Si tienes un mdulo SH1106, la librera Adafruit_SSD1306 puede funcionar
// pero es ms recomendable usar la librera "U8g2" con el constructor:
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  Serial.begin(115200);

  // Inicializa I2C (SDA y SCL predeterminados en Feather RP2040 CAN)
  // SDA = Pin 2, SCL = Pin 3 en la Feather RP2040
  Wire.begin();

  // SSD1306_SWITCHCAPVCC = genera el voltaje de la pantalla internamente
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Error: Fall la inicializacin de la pantalla OLED"));
    for(;;); // No continuar si falla
  }

  // Limpia el buffer inicial de la librera (logo de Adafruit)
  display.clearDisplay();

  // Muestra mensaje de bienvenida
  display.setTextSize(1);      
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("FEATHER RP2040 CAN"));
  display.println(F(" OLED INIZIALIZED"));
  display.println(F("------------------"));
  display.setTextSize(2);
  display.println(F("  HOLA!"));
  display.display();
  
  delay(2000);
}

void loop() {
  display.clearDisplay();
  
  // Ejemplo de visualizacin de datos
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("System: OK"));
  
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print(F("Uptime:"));
  
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print(millis() / 1000);
  display.println(F(" seconds"));
  
  // Dibuja una lnea decorativa
  display.drawLine(0, 60, 127, 60, SSD1306_WHITE);
  
  display.display(); // Importante: actualiza la pantalla con los datos del buffer
  delay(500);
}
