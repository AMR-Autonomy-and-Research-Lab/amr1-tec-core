import type { CanvasContext } from "../../types"

type PatternRepetition = "repeat" | "repeat-x" | "repeat-y" | "no-repeat" | null

interface PatternCapableContext extends CanvasContext {
  createPattern(
    image: CanvasImageSource,
    repetition: PatternRepetition,
  ): CanvasPattern | null
}

function isPatternCapableContext(
  ctx: CanvasContext,
): ctx is PatternCapableContext {
  return (
    "createPattern" in ctx &&
    typeof Reflect.get(ctx, "createPattern") === "function"
  )
}

export function mergeSoldermaskLayer(
  baseCtx: CanvasContext,
  soldermaskCtx: CanvasContext,
): void {
  if (baseCtx === soldermaskCtx) return
  if (soldermaskCtx.canvas.width <= 0 || soldermaskCtx.canvas.height <= 0)
    return
  if (!isPatternCapableContext(baseCtx)) return

  let pattern: CanvasPattern | null = null
  try {
    pattern = baseCtx.createPattern(
      soldermaskCtx.canvas as CanvasImageSource,
      "no-repeat",
    )
  } catch {
    return
  }
  if (!pattern) return

  baseCtx.save()
  baseCtx.globalCompositeOperation = "source-over"
  baseCtx.fillStyle = pattern
  baseCtx.fillRect(
    0,
    0,
    soldermaskCtx.canvas.width,
    soldermaskCtx.canvas.height,
  )
  baseCtx.restore()
}
