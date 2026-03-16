import React from "react"
import { CadViewer } from "@tscircuit/3d-viewer"

/**
 * Visor 3D de la PCB AMR usando @tscircuit/3d-viewer
 * https://github.com/tscircuit/3d-viewer
 * Dimensiones desde PCB_Final.json (EasyEDA): ~92mm x 88mm
 */
const circuitJson = [
  {
    type: "pcb",
    id: "pcb_amr",
    board_width: "92mm",
    board_height: "88mm",
  },
]

function App() {
  return (
    <div style={{ width: "100vw", height: "100vh", background: "#1a1a2e" }}>
      <CadViewer circuitJson={circuitJson} />
    </div>
  )
}

export default App
