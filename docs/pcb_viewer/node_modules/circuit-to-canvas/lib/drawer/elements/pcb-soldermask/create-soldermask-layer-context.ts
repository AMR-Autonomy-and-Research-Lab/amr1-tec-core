import type { CanvasContext } from "../../types"

interface SoldermaskCanvas {
  width: number
  height: number
  getContext(contextId: "2d"): CanvasContext | null
}

type SoldermaskCanvasConstructor = new (
  width: number,
  height: number,
) => SoldermaskCanvas

function isCanvasConstructor(
  value: unknown,
): value is SoldermaskCanvasConstructor {
  return typeof value === "function"
}

export function createSoldermaskLayerContext(
  baseCtx: CanvasContext,
  width: number,
  height: number,
): CanvasContext | null {
  if (width <= 0 || height <= 0) return null

  const g = globalThis
  let layerCanvas: SoldermaskCanvas | null = null

  if ("OffscreenCanvas" in g && typeof g.OffscreenCanvas === "function") {
    layerCanvas = new g.OffscreenCanvas(width, height)
  } else if (
    "document" in g &&
    g.document &&
    typeof g.document.createElement === "function"
  ) {
    const canvas = g.document.createElement("canvas")
    canvas.width = width
    canvas.height = height
    layerCanvas = canvas
  } else {
    const CanvasCtor = baseCtx.canvas.constructor
    if (isCanvasConstructor(CanvasCtor)) {
      try {
        layerCanvas = new CanvasCtor(width, height)
      } catch {
        return null
      }
    }
  }

  if (!layerCanvas) return null
  return layerCanvas.getContext("2d")
}
