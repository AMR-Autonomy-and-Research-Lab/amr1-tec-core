import type { PcbPlatedHole, PcbTrace, PcbVia } from "circuit-json"
import type { Matrix } from "transformation-matrix"
import type { CanvasContext } from "../../types"
import { drawLine } from "../../shapes/line"
import { drawPolygon } from "../../shapes/polygon"
import { buildTracePolygon } from "../pcb-trace/build-trace-polygon"
import { collectTraceSegments } from "../pcb-trace/collect-trace-segments"
import { cutTraceDestinationsAtDrills } from "../pcb-trace/cut-trace-destination-drills"
import { hasVariableWidth } from "../pcb-trace/has-variable-width"

export function processTraceSoldermask(params: {
  ctx: CanvasContext
  trace: PcbTrace
  realToCanvasMat: Matrix
  soldermaskOverCopperColor: string
  layer: "top" | "bottom"
  vias: PcbVia[]
  platedHoles: PcbPlatedHole[]
}): void {
  const { ctx, trace, realToCanvasMat, soldermaskOverCopperColor, layer } =
    params
  if (!trace.route || !Array.isArray(trace.route) || trace.route.length < 2) {
    return
  }

  const segments = collectTraceSegments(trace.route)

  for (const segment of segments) {
    const segmentLayer = segment[0]?.layer
    if (segmentLayer !== layer) continue

    if (hasVariableWidth(segment)) {
      const polygonPoints = buildTracePolygon(segment)
      drawPolygon({
        ctx,
        points: polygonPoints,
        fill: soldermaskOverCopperColor,
        realToCanvasMat,
      })
      continue
    }

    for (let i = 0; i < segment.length - 1; i++) {
      const start = segment[i]
      const end = segment[i + 1]
      if (!start || !end) continue

      drawLine({
        ctx,
        start: { x: start.x, y: start.y },
        end: { x: end.x, y: end.y },
        strokeWidth: start.width,
        stroke: soldermaskOverCopperColor,
        realToCanvasMat,
        lineCap: "round",
      })
    }
  }

  cutTraceDestinationsAtDrills({
    ctx,
    trace,
    realToCanvasMat,
    layer,
    vias: params.vias,
    platedHoles: params.platedHoles,
  })
}
