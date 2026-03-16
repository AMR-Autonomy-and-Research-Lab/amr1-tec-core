import type { PcbVia } from "circuit-json"
import type { Matrix } from "transformation-matrix"
import { applyToPoint } from "transformation-matrix"
import type { CanvasContext } from "../../types"
import { cutPathFromSoldermask } from "./cut-path-from-soldermask"

/**
 * Process soldermask for a via.
 * Vias typically have soldermask openings to expose the copper ring.
 */
export function processViaSoldermask(params: {
  ctx: CanvasContext
  via: PcbVia
  realToCanvasMat: Matrix
}): void {
  const { ctx, via, realToCanvasMat } = params
  // Vias typically have soldermask openings to expose the copper ring.
  const [cx, cy] = applyToPoint(realToCanvasMat, [via.x, via.y])
  const scaledRadius = (via.outer_diameter / 2) * Math.abs(realToCanvasMat.a)

  ctx.beginPath()
  ctx.arc(cx, cy, scaledRadius, 0, Math.PI * 2)
  ctx.closePath()
  cutPathFromSoldermask(ctx)
}
