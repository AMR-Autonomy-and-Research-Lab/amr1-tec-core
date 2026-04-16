/**
 * AMR1 - ADVANCED VEHICLE CONTROL DASHBOARD
 * Hardware: Adafruit Feather RP2040 CAN
 * Logic: Based on AMR TEC code (Brakes, Throttle, Gears, Steering)
 *
 * This code runs on the RP2040 dual-core architecture.
 * Core 1: Handles high-speed SBUS decoding and signal inversion.
 * Core 0: Manages the professional Serial diagnostic dashboard.
 */

#include "hardware/gpio.h"
#include "hardware/uart.h"
#include <Arduino.h>

// --- CONFIGURATION ---
#define SBUS_PIN_RX 1
#define BAUD_SBUS 100000
#define SBUS_FRAME_SIZE 25

// --- GLOBAL STATE ---
struct SBUS_State {
  uint16_t ch[16]; // All 16 channels
  bool failsafe;
  unsigned long last_update;

  // Computed Vehicle Logic (Mapeo solicitado)
  int valorVel;
  int valorDir;
  int valorFreno;
  int valorEnable;
  int valorReversa;
  bool highGear;
};

// State shared between cores
volatile SBUS_State vehicle;

// --- DECODING LOGIC ---
void parseSBUS(uint8_t *packet) {
  // Decode 16 channels (11-bit each) from the 25-byte packet
  vehicle.ch[0] = ((packet[1] | packet[2] << 8) & 0x07FF);
  vehicle.ch[1] = ((packet[2] >> 3 | packet[3] << 5) & 0x07FF);
  vehicle.ch[2] =
      ((packet[3] >> 6 | packet[4] << 2 | packet[5] << 10) & 0x07FF);
  vehicle.ch[3] = ((packet[5] >> 1 | packet[6] << 7) & 0x07FF);
  vehicle.ch[4] = ((packet[6] >> 4 | packet[7] << 4) & 0x07FF);
  vehicle.ch[5] = ((packet[7] >> 7 | packet[8] << 1 | packet[9] << 9) & 0x07FF);
  vehicle.ch[6] = ((packet[9] >> 2 | packet[10] << 6) & 0x07FF);
  vehicle.ch[7] = ((packet[10] >> 5 | packet[11] << 3) & 0x07FF);
  vehicle.ch[8] = ((packet[12] | packet[13] << 8) & 0x07FF);
  vehicle.ch[9] = ((packet[13] >> 3 | packet[14] << 5) & 0x07FF);
  vehicle.ch[10] =
      ((packet[14] >> 6 | packet[15] << 2 | packet[16] << 10) & 0x07FF);
  vehicle.ch[11] = ((packet[16] >> 1 | packet[17] << 7) & 0x07FF);
  vehicle.ch[12] = ((packet[17] >> 4 | packet[18] << 4) & 0x07FF);
  vehicle.ch[13] =
      ((packet[18] >> 7 | packet[19] << 1 | packet[20] << 9) & 0x07FF);
  vehicle.ch[14] = ((packet[20] >> 2 | packet[21] << 6) & 0x07FF);
  vehicle.ch[15] = ((packet[21] >> 5 | packet[22] << 3) & 0x07FF);

  vehicle.failsafe = packet[23] & 0x08;

  // --- INTEGRATED VEHICLE LOGIC (Mapeo del usuario) ---

  // Logic for CH0: Throttle & Brake
  if (vehicle.ch[0] < 980) {
    vehicle.valorFreno = map(vehicle.ch[0], 980, 172, 150, 179);
    vehicle.valorVel = 0;
  } else if (vehicle.ch[0] >= 980 && vehicle.ch[0] <= 1000) {
    vehicle.valorVel = 0;
    vehicle.valorFreno = 150;
  } else {
    vehicle.valorVel = map(vehicle.ch[0], 1000, 1811, 50, 200);
    vehicle.valorFreno = 150;
  }

  // Logic for CH1: Steering
  vehicle.valorDir = map(vehicle.ch[1], 172, 1811, 100, 0);

  // Logic for CH2: Gears (Low/High)
  vehicle.highGear = (vehicle.ch[2] > 1000);

  // Logic for CH3: Reverse (1 = Active)
  vehicle.valorReversa = (vehicle.ch[3] < 980) ? 1 : 0;

  // Logic for CH4: Enable / Emergency Brake
  if (vehicle.ch[4] < 980) {
    vehicle.valorFreno = 179; // Freno a tope
    vehicle.valorEnable = 0;
  } else {
    vehicle.valorEnable = 1;
  }

  if (!vehicle.failsafe)
    vehicle.last_update = millis();
}

// --- CORE 1: RX BACKGROUND TASK ---
void setup1() {
  // Configuración UART para SBUS (8E2) e inversión por registro
  Serial1.begin(BAUD_SBUS, SERIAL_8E2);
  gpio_set_inover(SBUS_PIN_RX, 1); // 1 = Invert (GPIO_OVERRIDE_INVERT)
}

void loop1() {
  static uint8_t buf[25];
  static int idx = 0;
  while (Serial1.available()) {
    uint8_t c = Serial1.read();
    if (idx == 0 && c != 0x0F)
      continue;
    buf[idx++] = c;
    if (idx == 25) {
      if (buf[24] == 0x00)
        parseSBUS(buf);
      idx = 0;
    }
  }
}

// --- CORE 0: DASHBOARD UI ---
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000)
    ;
}

void drawBar(int val, int minV, int maxV, int width) {
  int pos = map(val, minV, maxV, 0, width);
  Serial.print("[");
  for (int i = 0; i < width; i++) {
    if (i < pos)
      Serial.print("=");
    else
      Serial.print(" ");
  }
  Serial.print("]");
}

void loop() {
  static unsigned long lastUI = 0;
  if (millis() - lastUI < 150)
    return; // Update display frequency
  lastUI = millis();

  // Clear terminal with ANSI escape codes (works in VS Code, PlatformIO, PuTTY)
  Serial.print("\033[2J\033[H");

  Serial.println(
      "===============================================================");
  Serial.println(
      "     AMR1 - FR-SKY TARANIS Q X7 DASHBOARD (RP2040 CAN)        ");
  Serial.println(
      "===============================================================");

  if (vehicle.failsafe || (millis() - vehicle.last_update > 500)) {
    Serial.println("\n [!!!] SIGNAL LOST - CONNECT TRANSMITTER [!!!]");
    return;
  }

  // --- STICKS MONITOR (CH1-4) ---
  Serial.print(" STICKS: ");
  Serial.print("CH1:");
  Serial.print(vehicle.ch[0]);
  Serial.print("  ");
  Serial.print("CH2:");
  Serial.print(vehicle.ch[1]);
  Serial.print("  ");
  Serial.print("CH3:");
  Serial.print(vehicle.ch[2]);
  Serial.print("  ");
  Serial.print("CH4:");
  Serial.print(vehicle.ch[3]);
  Serial.println();

  // --- VEHICLE OUTPUTS (Mapped Values) ---
  Serial.println("\n --- COMPUTED VEHICLE STATE ---");

  Serial.print(" VELOCITY (CH1):  ");
  drawBar(vehicle.valorVel, 0, 200, 20);
  Serial.print("  Out: ");
  Serial.println(vehicle.valorVel);

  Serial.print(" STEERING (CH2):  ");
  drawBar(vehicle.valorDir, 0, 100, 20);
  Serial.print("  Out: ");
  Serial.println(vehicle.valorDir);

  Serial.print(" BRAKE PRESSURE:  ");
  drawBar(vehicle.valorFreno, 150, 179, 20);
  Serial.print("  Out: ");
  Serial.println(vehicle.valorFreno);

  Serial.println("\n --- LOGIC STATUS ---");
  Serial.print(" [ENABLE: ");
  Serial.print(vehicle.valorEnable ? "YES" : "NO ");
  Serial.print("]  [REVERSE: ");
  Serial.print(vehicle.valorReversa ? "YES" : "NO ");
  Serial.print("]  [GEAR: ");
  Serial.print(vehicle.highGear ? "HIGH" : "LOW ");
  Serial.println("]");

  // --- AUXILIARY CHANNELS (CH5-CH16) ---
  Serial.println("\n --- AUXILIARY CHANNELS (Buttons/Switches) ---");
  for (int i = 4; i < 16; i++) {
    Serial.print("CH");
    Serial.print(i + 1);
    Serial.print(":");
    if (i < 9)
      Serial.print(" "); // Padding
    Serial.print(vehicle.ch[i]);
    if ((i + 1) % 4 == 0)
      Serial.println();
    else
      Serial.print("\t");
  }
  Serial.println(
      "\n===============================================================");
}
