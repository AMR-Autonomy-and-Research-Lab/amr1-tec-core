<div align="center">

<br/>

<img src="https://img.shields.io/badge/AMR%20Autonomy%20%26%20Research%20Lab-Tecnológico%20de%20Monterrey-0a1628?style=for-the-badge&labelColor=0a1628&color=003d7a&logoColor=white" />

<br/><br/>

# AMR1-TEC-CORE

### Embedded Hardware Control Platform for Autonomous Mobile Robotics

<br/>

[![License: MIT](https://img.shields.io/badge/License-MIT-00f0ff?style=flat-square)](LICENSE)
[![MCU](https://img.shields.io/badge/MCU-RP2040_CAN-ff0055?style=flat-square&logo=raspberrypi&logoColor=white)](https://www.adafruit.com/product/5709)
[![CAN Bus](https://img.shields.io/badge/Bus-CAN_500_kbps-00cc44?style=flat-square)](https://en.wikipedia.org/wiki/CAN_bus)
[![EDA](https://img.shields.io/badge/EDA-EasyEDA-f5a623?style=flat-square)](https://easyeda.com)
[![3D CAD](https://img.shields.io/badge/3D_CAD-Fusion_360-0696D7?style=flat-square&logo=autodesk&logoColor=white)](https://www.autodesk.com/products/fusion-360)

<br/>

> Custom PCB that unifies perception, closed-loop actuator control, CAN bus distribution, and real-time telemetry in a single board — engineered to production standards for AMR research at Tec de Monterrey.

<br/>

</div>

---

## ⚡ Live Interactive Viewers

<div align="center">
<br/>

**Explore the full PCB geometry, component placement, and netlist — directly in your browser. No installation required.**

<br/>

<a href="https://htmlpreview.github.io/?https://raw.githubusercontent.com/AMR-Autonomy-and-Research-Lab/amr1-tec-core/main/docs/hardware/3d_viewer/index.html">
  <img src="https://img.shields.io/badge/🧊_OPEN_3D_VIEWER-Rotate_%7C_Zoom_%7C_Inspect_the_PCB_live-ff0055?style=for-the-badge" alt="Open 3D Viewer" height="40"/>
</a>

<br/><br/>

<a href="https://htmlpreview.github.io/?https://raw.githubusercontent.com/AMR-Autonomy-and-Research-Lab/amr1-tec-core/main/docs/hardware/ibom_final.html">
  <img src="https://img.shields.io/badge/⚡_OPEN_IBOM_MAP-Click_any_component_to_locate_it_on_the_PCB-00f0ff?style=for-the-badge" alt="Open iBOM" height="40"/>
</a>

<br/><br/>

| Feature | 3D Spatial Viewer | iBOM Component Map |
|:-------:|:-----------------:|:-----------------:|
| **Technology** | WebGL · Three.js | Interactive HTML |
| **Controls** | Orbit · Zoom · Pan | Click to highlight |
| **Use case** | Visual inspection, renders | Assembly, soldering guide |

<br/>
</div>

---

## PCB Preview

<div align="center">

> **Drop your renders in `docs/hardware/images/` to display them here.**

<!-- Uncomment and replace paths once images are available:

### 3D Render
<a href="https://htmlpreview.github.io/?https://raw.githubusercontent.com/AMR-Autonomy-and-Research-Lab/amr1-tec-core/main/docs/hardware/3d_viewer/index.html">
  <img src="docs/hardware/images/pcb_3d.png" alt="PCB 3D — click to open interactive viewer" width="860"/>
</a>
<sub><i>↑ Click the image to open the interactive 3D viewer</i></sub>

### PCB Layout & Routing
<img src="docs/hardware/images/pcb_ruteo.png" alt="PCB Routing" width="860"/>

### Schematic
<img src="docs/hardware/images/esquematico.png" alt="Schematic" width="860"/>

-->

</div>

---

## Technical Specifications

<div align="center">

### Control & Communication

| Parameter | Value |
|-----------|-------|
| Microcontroller | Adafruit Feather RP2040 CAN |
| CAN Controller | MCP2515 (SPI) |
| CAN Bus Speed | 500 kbps |
| Serial Input | UART 115200 baud |
| Form Factor | Feather-compatible |

### Linear Actuator Control — Closed Loop

| Parameter | Value |
|-----------|-------|
| Actuator | Pololu Glideforce High-Speed LD |
| Rated Load | 12 kgf |
| Stroke | 6 in |
| Supply | 12 V |
| Control | PWM + Direction + FLT feedback |
| Position Feedback | Potentiometer (ADC) |

### On-Board Telemetry Display

| Parameter | Value |
|-----------|-------|
| Display | SSD1306 OLED 128×64 |
| Interface | I²C |
| Shows | CAN status · PWM duty · current · FLT diagnostics |

</div>

---

## System Architecture

```
┌──────────────────────────────────────────────────┐
│                  AMR1-TEC-CORE                   │
│                                                  │
│  UART In (115200 baud)                           │
│       │                                          │
│  ┌────▼────────────────────┐                     │
│  │   Feather RP2040 CAN    │    ┌──────────────┐ │
│  │   MCP2515 @ SPI         │    │ SSD1306 OLED │ │
│  └──┬──────────────┬───────┘    │  128×64 I²C  │ │
│     │              └────────────►  Telemetry   │ │
│   CAN Bus 500 kbps              └──────────────┘ │
│     │                                            │
│  ┌──▼──────────────────────┐                     │
│  │  CAN RX Node (RP2040)   │                     │
│  └──┬──────────────────────┘                     │
│     │                                            │
│  PWM + DIR + FLT pin                             │
│     │                                            │
│  ┌──▼──────────────────────┐                     │
│  │  Linear Actuator         │                    │
│  │  Pololu Glideforce LD    │◄── ADC pot feedback│
│  │  12 kgf · 12 V · 6 in   │                    │
│  └──────────────────────────┘                    │
└──────────────────────────────────────────────────┘
```

---

## Repository Structure

```
amr1-tec-core/
├── PCB_Design/
│   ├── PCB_AMR_Gerber/          # Validated Gerbers — ready for fab
│   └── PCB_AMR_Gerber.zip       # Drop into JLCPCB or PCBWay directly
├── 3D_Models/
│   └── PCB_3D/
│       ├── PCB_Final.glb        # Optimized 3D model for WebGL viewer
│       └── OBJ_PCB_Final.obj    # Raw OBJ export from EasyEDA
├── feather_can_tx/              # Firmware: Serial → CAN transmitter
├── feather_can_rx/              # Firmware: CAN RX + actuator control
├── Diseño_CAJA/                 # Enclosure STL models
└── docs/hardware/
    ├── ibom_final.html          # Interactive Bill of Materials
    └── 3d_viewer/
        └── index.html           # Self-contained WebGL 3D viewer
```

---

## PCB Fabrication

Files are validated and ready to order. No special configuration needed.

1. Go to `PCB_Design/PCB_AMR_Gerber/` or grab `PCB_AMR_Gerber.zip`
2. Upload to [JLCPCB](https://jlcpcb.com) or [PCBWay](https://www.pcbway.com)
3. Standard 2-layer, commercial tolerances, PTH/NPTH drills, dual solder mask

---

<div align="center">
<br/>

Designed by the **AMR Autonomy and Research Lab** · Tecnológico de Monterrey

[![MIT License](https://img.shields.io/badge/License-MIT-00f0ff?style=flat-square)](LICENSE)

<br/>
</div>
