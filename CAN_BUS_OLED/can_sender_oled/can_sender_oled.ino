/*
 * ============================================
 *   FEATHER RP2040 CAN — SENDER
 *   Envía mensajes CAN cada ~1 segundo
 *   (Igual que tu sender original)
 * ============================================
 */

#include <Adafruit_MCP2515.h>

#define CS_PIN       PIN_CAN_CS
#define CAN_BAUDRATE 500000

#define LED_PIN LED_BUILTIN

Adafruit_MCP2515 mcp(CS_PIN, &SPI1);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  pinMode(LED_PIN, OUTPUT);

  Serial.println("============================");
  Serial.println(" CAN SENDER + OLED SYSTEM");
  Serial.println("============================");

  // SPI1 para el MCP2515
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

  if (!mcp.begin(CAN_BAUDRATE)) {
    Serial.println("ERROR: MCP2515 no responde");
    while (1)
      ;
  }

  Serial.println("CAN OK — Sender listo");
  Serial.println();
}

void loop() {
  // 🔵 Encender LED al enviar
  digitalWrite(LED_PIN, HIGH);

  // Mensaje ID 0x101
  mcp.beginPacket(0x101);
  mcp.print("MSG101");
  mcp.endPacket();

  // Mensaje ID 0x102
  mcp.beginPacket(0x102);
  mcp.print("MSG102");
  mcp.endPacket();

  Serial.println("Mensajes enviados (0x101 + 0x102)");

  delay(100);

  // 🔴 Apagar LED
  digitalWrite(LED_PIN, LOW);

  delay(900); // total ~1 segundo
}
