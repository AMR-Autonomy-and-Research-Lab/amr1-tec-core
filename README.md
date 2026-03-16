<div align="center">
  <br />
  <img src="https://img.shields.io/badge/AMR-Autonomy%20%26%20Research%20Lab-0D0F14?style=for-the-badge&logo=appveyor" alt="AMR Lab" />
  <h1>AMR1-TEC-CORE</h1>
  <p><strong>Plataforma integral de hardware para investigación en Robótica Móvil Autónoma</strong></p>
  <br />
</div>

> **AMR1-TEC-CORE** es el cerebro electrónico que integra percepción, navegación, control de actuadores y sistemas embebidos en un diseño único. Desarrollado con estándares de industria para ambientes experimentales y robóticos.

---

## ⚡ Diseño de Hardware y PCB (Dashboard Interactivo)

Hemos superado los diagramas estáticos tradicionales. Puedes explorar cada componente, pista y pad de soldadura de nuestra PCB de control de manera completamente interactiva:

<div align="center">
  <br />
  <a href="https://htmlpreview.github.io/?https://raw.githubusercontent.com/AMR-Autonomy-and-Research-Lab/amr1-tec-core/main/docs/hardware/ibom_final.html">
    <img src="https://img.shields.io/badge/⚡_Abrir_Mapa_Interactivo_de_la_Placa_PCB-VER_AHORA-00f0ff?style=for-the-badge&logo=github" alt="Abrir Mapa Interactivo" />
  </a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://htmlpreview.github.io/?https://raw.githubusercontent.com/AMR-Autonomy-and-Research-Lab/amr1-tec-core/main/docs/hardware/3d_viewer/index.html">
    <img src="https://img.shields.io/badge/📦_Abrir_Visor_Espacial_3D_de_Geometría-ROTAR_AHORA-ff0055?style=for-the-badge&logo=threedotjs" alt="Abrir Visor 3D" />
  </a>
  <br /><br />
  <p><small>(Visores interactivos online creados desde cero — Renderizado directo en tu navegador)</small></p>
  <br />
</div>

---

## 📸 Renderizados y Arquitectura

Guarda las fotos de tu placa en la carpeta `docs/hardware/images/` con los nombres indicados para que se visualicen aquí en el repositorio de manera ultra profesional:

<div align="center">
  
### Render 3D 
<img src="docs/hardware/images/pcb_3d.png" alt="Render 3D de la PCB" width="800" onerror="this.src='https://via.placeholder.com/800x400/000000/00f0ff?text=Guarda+tu+render+azul+como+docs/hardware/images/pcb_3d.png'" />

### Ruteo de Hardware
<img src="docs/hardware/images/pcb_ruteo.png" alt="PCB Route" width="800" onerror="this.src='https://via.placeholder.com/800x400/000000/00f0ff?text=Guarda+tu+ruteo+como+docs/hardware/images/pcb_ruteo.png'" />

### Esquemático
<img src="docs/hardware/images/esquematico.png" alt="Esquematico" width="800" onerror="this.src='https://via.placeholder.com/800x400/000000/00f0ff?text=Guarda+tu+esquematico+como+docs/hardware/images/esquematico.png'" />

</div>

---

## 📦 Arquitectura del Proyecto

```text
amr1-tec-core/
├── 📂 PCB_Design/              # Diseño electrónico en EasyEDA y archivos Gerber
├── 📂 feather_can_tx/          # Transmisor de Comandos Serial → CAN (115200 a 500 kbps)
├── 📂 feather_can_rx/          # Receptor CAN y cerebro de motor y actuadores Pololu
├── 📂 docs/hardware/           # Entorno Web del visor iBom para soldadura interactiva
└── 📂 Diseño_CAJA/             # Modelos 3D (.STL) de la carcasa protectora
```

## 🛠 Especificaciones Técnicas

### Control Central y Comunicaciones
*   Microcontrolador: **Adafruit Feather RP2040 CAN** (MCP2515) operando a 500 kbps.
*   Protocolos robustos de comunicación para distribución de comandos sin pérdida de latencia.

### Actuador Lineal y Frenos (Lazo Cerrado)
*   **Pololu Glideforce High-Speed LD** (12 kgf, carrera 6", 12V).
*   Control vectorial PWM y Dirección con protecciones (`FLT`).
*   Feedback absoluto por potenciómetro en pin analógico.

### Display de Telemetría On-Board
*   **SSD1306 OLED** 128×64 I2C. Provee telemetría en tiempo real: Red CAN, PWM, corriente y diagnóstico de fallas (FLT).

---

## 🚀 Fabricación Inmediata

Si quieres mandar a imprimir la placa en JLCPCB o PCBWay de inmediato, simplemente usa nuestros gerbers validados:

1. Ve a la carpeta `PCB_Design/PCB_AMR_Gerber/` o descarga nuestro `PCB_AMR_Gerber.zip`
2. Arrástralo a JLCPCB.
3. El diseño usa márgenes comerciales, taladros PTH/NPTH estandarizados y máscara superior/inferior. Está listo para mandar a la fábrica.

---

> Diseñado por el **AMR Autonomy and Research Lab** — Tecnológico de Monterrey  
> Distribuido bajo licencia MIT.
