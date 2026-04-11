/*
 * ============================================
 *   FEATHER RP2040 CAN — RECEIVER (POR ID)
 *   Escucha SOLO un ID específico (configurable)
 * ============================================
 */

#include <Adafruit_MCP2515.h>

#define CAN_BAUDRATE 500000

// 🔧 CONFIGURA AQUÍ EL ID QUE QUIERES ESCUCHAR
#define MY_CAN_ID 0x102 // <-- cámbialo a 0x102, 0x111, etc.

Adafruit_MCP2515 mcp(PIN_CAN_CS, &SPI1);

uint32_t rx_total = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("============================");
  Serial.println(" RECEIVER POR ID");
  Serial.print(" Escuchando ID: 0x");
  Serial.println(MY_CAN_ID, HEX);
  Serial.println("============================");

  // SPI1
  SPI1.setRX(8);
  SPI1.setTX(15);
  SPI1.setSCK(14);
  SPI1.begin(false);

  // Activar CAN
  pinMode(PIN_CAN_STANDBY, OUTPUT);
  digitalWrite(PIN_CAN_STANDBY, LOW);

  pinMode(PIN_CAN_RESET, OUTPUT);
  digitalWrite(PIN_CAN_RESET, HIGH);

  delay(10);

  if (!mcp.begin(CAN_BAUDRATE)) {
    Serial.println("ERROR: MCP25625 no responde");
    while (1)
      ;
  }

  Serial.println("CAN OK");
  Serial.println();
}

void loop() {
  int len = mcp.parsePacket();

  if (len > 0) {
    uint32_t id = mcp.packetId();

    if (id != MY_CAN_ID)
      return;

    Serial.print("[RX ID 0x");
    Serial.print(id, HEX);
    Serial.print("] Data: ");

    while (mcp.available()) {
      uint8_t val = mcp.read();
      Serial.print(val);
      Serial.print(" ");
    }

    Serial.println();

    rx_total++;

    digitalWrite(LED_BUILTIN, HIGH);
    delay(20);
    digitalWrite(LED_BUILTIN, LOW);
  }
}