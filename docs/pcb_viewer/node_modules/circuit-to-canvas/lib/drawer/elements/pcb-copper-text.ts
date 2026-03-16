import type { NinePointAnchor, PcbCopperText } from "circuit-json"
import type { Matrix } from "transformation-matrix"
import { applyToPoint } from "transformation-matrix"
import {
  getAlphabetLayout,
  getTextStartPosition,
  strokeAlphabetText,
} from "../shapes/text"
import type { CanvasContext, PcbColorMap } from "../types"

export interface DrawPcbCopperTextParams {
  ctx: CanvasContext
  text: PcbCopperText
  realToCanvasMat: Matrix
  colorMap: PcbColorMap
}

const DEFAULT_PADDING = { left: 0.2, right: 0.2, top: 0.2, bottom: 0.2 }

function mapAnchorAlignment(alignment?: string): NinePointAnchor {
  // Vertical component is intentionally collapsed to center; callers only care about left/center/right.
  if (!alignment) return "center"
  if (alignment.includes("left")) return "center_left"
  if (alignment.includes("right")) return "center_right"
  return "center"
}

export function drawPcbCopperText(params: DrawPcbCopperTextParams): void {
  const { ctx, text, realToCanvasMat, colorMap } = params

  const content = text.text ?? ""
  if (!content) return

  const [x, y] = applyToPoint(realToCanvasMat, [
    text.anchor_position.x,
    text.anchor_position.y,
  ])
  const scale = Math.abs(realToCanvasMat.a)
  const fontSize = (text.font_size ?? 1) * scale
  const rotation = text.ccw_rotation ?? 0
  const padding = {
    ...DEFAULT_PADDING,
    ...text.knockout_padding,
  }
  const textColor =
    colorMap.copper[text.layer as keyof typeof colorMap.copper] ??
    colorMap.copper.top
  const layout = getAlphabetLayout(content, fontSize)
  const totalWidth = layout.width + layout.strokeWidth
  const alignment = mapAnchorAlignment(text.anchor_alignment)
  const startPos = getTextStartPosition(alignment, layout)

  ctx.save()
  ctx.translate(x, y)
  if (text.is_mirrored) ctx.scale(-1, 1)
  if (rotation !== 0) ctx.rotate(-rotation * (Math.PI / 180))

  ctx.lineWidth = layout.strokeWidth
  ctx.lineCap = "round"
  ctx.lineJoin = "round"

  if (text.is_knockout) {
    const paddingLeft = padding.left * scale
    const paddingRight = padding.right * scale
    const paddingTop = padding.top * scale
    const paddingBottom = padding.bottom * scale
    const rectX = startPos.x - paddingLeft * 4
    const rectY = startPos.y - paddingTop * 4
    const rectWidth = totalWidth + paddingLeft * 2 + paddingRight * 2
    const rectHeight =
      layout.height + layout.strokeWidth + paddingTop * 2 + paddingBottom * 2

    // Draw knockout rectangle
    ctx.fillStyle = textColor
    ctx.fillRect(rectX, rectY, rectWidth, rectHeight)
  } else {
    ctx.strokeStyle = textColor
  }

  strokeAlphabetText({
    ctx,
    text: content,
    fontSize,
    startX: startPos.x,
    startY: startPos.y,
    anchorAlignment: alignment,
  })
  ctx.restore()
}
