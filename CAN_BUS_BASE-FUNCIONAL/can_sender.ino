#include <Adafruit_MCP2515.h>

#define CS_PIN PIN_CAN_CS
#define CAN_BAUDRATE 250000

#define LED_PIN LED_BUILTIN

Adafruit_MCP2515 mcp(CS_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(LED_PIN, OUTPUT);

  if (!mcp.begin(CAN_BAUDRATE)) {
    Serial.println("Error MCP2515");
    while (1)
      ;
  }

  Serial.println("Sender listo");
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

  Serial.println("Mensajes enviados");

  delay(100); // LED visible

  // 🔴 Apagar LED después de enviar
  digitalWrite(LED_PIN, LOW);

  delay(900); // total ~1 segundo
}