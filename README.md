<div align="center">

<img src="https://img.shields.io/badge/AMR%20Autonomy%20%26%20Research%20Lab-Tecnológico%20de%20Monterrey-003366?style=for-the-badge&logoColor=white" alt="AMR Lab" />

# AMR1-TEC-CORE

**Embedded Control Platform for Autonomous Mobile Robotics Research**

[![License: MIT](https://img.shields.io/badge/License-MIT-00f0ff?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/MCU-RP2040%20CAN-ff0055?style=flat-square&logo=raspberrypi)](https://www.adafruit.com/product/5709)
[![Protocol](https://img.shields.io/badge/Bus-CAN%20500%20kbps-00cc44?style=flat-square)](https://en.wikipedia.org/wiki/CAN_bus)
[![EDA](https://img.shields.io/badge/EDA-EasyEDA-f5a623?style=flat-square)](https://easyeda.com)
[![CAD](https://img.shields.io/badge/3D%20CAD-Fusion%20360-0696D7?style=flat-square&logo=autodesk)](https://www.autodesk.com/products/fusion-360)

</div>

---

> **AMR1-TEC-CORE** is a custom-designed embedded control board that integrates perception, closed-loop actuator control, CAN bus distribution, and real-time telemetry display in a single unified PCB. Engineered to production standards for autonomous mobile robotics research at Tec de Monterrey.

---

## Interactive Hardware Dashboard

Explore the PCB beyond static images — use the live interactive viewers below:

<div align="center">

| Viewer | Description |
|--------|-------------|
| [**iBOM Interactive Map**](https://htmlpreview.github.io/?https://raw.githubusercontent.com/AMR-Autonomy-and-Research-Lab/amr1-tec-core/main/docs/hardware/ibom_final.html) | Click any component to locate it on the PCB layout |
| [**3D Spatial Viewer**](https://htmlpreview.github.io/?https://raw.githubusercontent.com/AMR-Autonomy-and-Research-Lab/amr1-tec-core/main/docs/hardware/3d_viewer/index.html) | Rotate, zoom, and inspect the full 3D PCB geometry |

</div>

---

## PCB Design Gallery

<div align="center">

### 3D Render
<img src="docs/hardware/images/pcb_3d.png" alt="PCB 3D Render" width="800" />

### PCB Layout & Routing
<img src="docs/hardware/images/pcb_ruteo.png" alt="PCB Routing" width="800" />

### Schematic
<img src="docs/hardware/images/esquematico.png" alt="Schematic" width="800" />

</div>

---

## Technical Specifications

### Central Processing & Communication

| Parameter | Value |
|-----------|-------|
| Microcontroller | Adafruit Feather RP2040 CAN |
| CAN Controller | MCP2515 (SPI) |
| CAN Bus Speed | 500 kbps |
| Firmware Serial Input | 115200 baud |
| Form Factor | Feather-compatible |

### Linear Actuator Control (Closed-Loop)

| Parameter | Value |
|-----------|-------|
| Actuator | Pololu Glideforce High-Speed LD |
| Load Rating | 12 kgf |
| Stroke Length | 6 in |
| Supply Voltage | 12 V |
| Control Method | PWM + Direction + FLT feedback |
| Position Feedback | Potentiometer (analog) |

### Telemetry Display

| Parameter | Value |
|-----------|-------|
| Display | SSD1306 OLED 128×64 |
| Interface | I²C |
| Data Shown | CAN bus status, PWM duty, current, FLT diagnostics |

---

## Repository Structure

```
amr1-tec-core/
├── PCB_Design/
│   ├── PCB_AMR_Gerber/        # Validated Gerber files ready for fabrication
│   └── PCB_AMR_Gerber.zip     # Pre-packaged Gerber ZIP for JLCPCB / PCBWay
├── 3D_Models/
│   └── PCB_3D/
│       ├── PCB_Final.glb      # Optimized 3D model (Three.js viewer)
│       └── OBJ_PCB_Final.obj  # Raw OBJ export from EasyEDA
├── feather_can_tx/            # Serial → CAN transmitter firmware (RP2040)
├── feather_can_rx/            # CAN receiver + actuator control firmware
├── docs/hardware/
│   ├── ibom_final.html        # Interactive Bill of Materials
│   └── 3d_viewer/
│       └── index.html         # WebGL 3D PCB viewer
└── Diseño_CAJA/               # Enclosure 3D models (.STL)
```

---

## PCB Fabrication

The Gerber files have been validated and are ready to order directly from JLCPCB or PCBWay:

1. Navigate to `PCB_Design/PCB_AMR_Gerber/` or download `PCB_AMR_Gerber.zip`
2. Upload the ZIP to [JLCPCB](https://jlcpcb.com) or [PCBWay](https://www.pcbway.com)
3. The design uses standard commercial tolerances, PTH/NPTH drill sizes, and dual-layer solder mask — no custom parameters required

---

## System Architecture

```
┌─────────────────────────────────────────────────────┐
│                   AMR1-TEC-CORE                      │
│                                                      │
│   Serial In (115200)                                 │
│        │                                             │
│   ┌────▼─────────────────┐                          │
│   │  Feather RP2040 CAN  │                          │
│   │  (MCP2515 @ SPI)     │                          │
│   └──┬──────────┬────────┘                          │
│      │          │                                    │
│   CAN Bus     I²C Bus                               │
│   500 kbps    SSD1306 OLED                          │
│      │                                               │
│   ┌──▼──────────────────┐                           │
│   │  CAN RX Node (RP2040)│                          │
│   └──┬──────────────────┘                           │
│      │                                               │
│   PWM + DIR + FLT                                   │
│      │                                               │
│   ┌──▼──────────────────┐                           │
│   │  Linear Actuator     │                          │
│   │  Pololu LD 12kgf     │◄── Pot feedback          │
│   └─────────────────────┘                           │
└─────────────────────────────────────────────────────┘
```

---

## License

Distributed under the [MIT License](LICENSE).

---

<div align="center">

Designed by the **AMR Autonomy and Research Lab** · Tecnológico de Monterrey

</div>
