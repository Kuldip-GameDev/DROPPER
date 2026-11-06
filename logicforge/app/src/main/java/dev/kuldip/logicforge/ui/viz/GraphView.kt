package dev.kuldip.logicforge.ui.viz

import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.TextMeasurer
import androidx.compose.ui.text.drawText
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.kuldip.logicforge.core.GraphNode
import dev.kuldip.logicforge.core.GraphSceneData
import dev.kuldip.logicforge.ui.theme.ForgeColors
import dev.kuldip.logicforge.ui.theme.Mono
import kotlin.math.abs
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.min
import kotlin.math.sin

private val NodeHeight = 44.dp

/** Nodes are pills as wide as their label (a circle for short labels). Monospace: ~8.4dp per char at 14sp. */
private fun nodeWidth(label: String): Dp = maxOf(NodeHeight, (label.length * 8.4f + 20f).dp)

/** Nodes and arrows. The core already chose x/y; we only scale them to the screen. */
@Composable
fun GraphView(data: GraphSceneData, modifier: Modifier = Modifier) {
    if (data.nodes.isEmpty()) {
        Text("(empty)", color = ForgeColors.TextDim, fontFamily = Mono)
        return
    }
    val minX = data.nodes.minOf { it.x }
    val maxX = data.nodes.maxOf { it.x }
    val minY = data.nodes.minOf { it.y }
    val maxY = data.nodes.maxOf { it.y }
    val top = if (data.pointers.isNotEmpty()) 40.dp else 12.dp
    val rowGap = 92.dp
    val height = top + rowGap * (maxY - minY).toFloat() + NodeHeight + 40.dp
    val widest = data.nodes.maxOf { nodeWidth(it.label) }
    val measurer = rememberTextMeasurer()

    if (data.title.isNotEmpty()) {
        Text(data.title, color = ForgeColors.TextDim, style = MaterialTheme.typography.labelMedium)
    }
    BoxWithConstraints(modifier.fillMaxWidth().height(height)) {
        val side = widest / 2 + 4.dp
        val spanX = (maxX - minX).toFloat()
        val unit: Dp = if (spanX == 0f) 0.dp else ((maxWidth - side * 2) / spanX).coerceAtMost(110.dp)
        val left = (maxWidth - unit * spanX) / 2
        fun cx(x: Double): Dp = left + unit * (x - minX).toFloat()
        fun cy(y: Double): Dp = top + NodeHeight / 2 + rowGap * (y - minY).toFloat()
        val byId = data.nodes.associateBy { it.id }
        val density = LocalDensity.current

        Canvas(Modifier.fillMaxSize()) {
            for (e in data.edges) {
                val a = byId[e.from] ?: continue
                val b = byId[e.to] ?: continue
                val color = when (e.mark) {
                    "none" -> ForgeColors.TextDim
                    "dim" -> ForgeColors.Outline
                    else -> ForgeColors.mark(e.mark)
                }
                // Curve an edge if it goes backwards along a row, or would pass through another node in that row.
                val sameRow = abs(a.y - b.y) < 1e-6
                val blocked = sameRow && data.nodes.any { n ->
                    n !== a && n !== b && abs(n.y - a.y) < 1e-6 && n.x > min(a.x, b.x) && n.x < maxOf(a.x, b.x)
                }
                with(density) {
                    drawEdge(
                        from = Offset(cx(a.x).toPx(), cy(a.y).toPx()),
                        to = Offset(cx(b.x).toPx(), cy(b.y).toPx()),
                        fromHalf = Offset((nodeWidth(a.label) / 2).toPx(), (NodeHeight / 2).toPx()),
                        toHalf = Offset((nodeWidth(b.label) / 2).toPx(), (NodeHeight / 2).toPx()),
                        curved = sameRow && (b.x < a.x || blocked),
                        directed = e.directed,
                        color = color,
                        label = e.label.takeUnless { it == "L" || it == "R" }.orEmpty(),
                        measurer = measurer,
                    )
                }
            }
        }
        for (n in data.nodes) NodeBox(n, cx(n.x), cy(n.y))
        for ((nodeId, ptrs) in data.pointers.groupBy { it.node }) {
            val n = byId[nodeId] ?: continue
            Text(
                ptrs.joinToString(",") { it.name } + "\n▼",
                Modifier.offset(cx(n.x) - 40.dp, cy(n.y) - NodeHeight / 2 - 34.dp).width(80.dp),
                color = ForgeColors.Gold, fontSize = 11.sp, textAlign = TextAlign.Center, fontFamily = Mono, lineHeight = 13.sp,
            )
        }
    }
}

@Composable
private fun NodeBox(n: GraphNode, cx: Dp, cy: Dp) {
    val bg by animateColorAsState(
        if (n.mark == "none") ForgeColors.SurfaceHigh else ForgeColors.mark(n.mark),
        label = "node",
    )
    val w = nodeWidth(n.label)
    // A wide fixed slot centred on (cx, cy); the pill inside is only as wide as it needs.
    Box(Modifier.offset(cx - w / 2, cy - NodeHeight / 2).width(w).height(NodeHeight), contentAlignment = Alignment.Center) {
        Box(
            Modifier.height(NodeHeight).widthIn(min = NodeHeight).clip(RoundedCornerShape(NodeHeight / 2)).background(bg).padding(horizontal = 8.dp),
            contentAlignment = Alignment.Center,
        ) {
            Text(
                n.label,
                color = if (n.mark == "none") ForgeColors.Text else ForgeColors.onMark(n.mark),
                fontFamily = Mono, fontWeight = FontWeight.Bold, fontSize = 14.sp, maxLines = 1,
            )
        }
    }
    if (n.sub.isNotEmpty()) {
        Text(
            n.sub,
            Modifier.offset(cx - 44.dp, cy + NodeHeight / 2 + 2.dp).width(88.dp),
            color = ForgeColors.TextDim, fontSize = 10.sp, textAlign = TextAlign.Center, fontFamily = Mono, lineHeight = 11.sp,
        )
    }
}

/** Where the line from a box's centre in direction `angle` leaves the box. */
private fun exitPoint(centre: Offset, half: Offset, angle: Float): Offset {
    val dx = cos(angle)
    val dy = sin(angle)
    val t = min(if (abs(dx) < 1e-6f) Float.MAX_VALUE else half.x / abs(dx), if (abs(dy) < 1e-6f) Float.MAX_VALUE else half.y / abs(dy))
    return Offset(centre.x + dx * t, centre.y + dy * t)
}

private fun DrawScope.drawEdge(
    from: Offset, to: Offset, fromHalf: Offset, toHalf: Offset,
    curved: Boolean, directed: Boolean, color: Color, label: String, measurer: TextMeasurer,
) {
    val path = Path()
    val tip: Offset
    val angle: Float
    val labelAt: Offset
    if (curved) {
        // Dip below the row: start and end on the bottom edges of the boxes.
        val dip = 54.dp.toPx()
        val start = Offset(from.x, from.y + fromHalf.y)
        val end = Offset(to.x, to.y + toHalf.y)
        val control = Offset((from.x + to.x) / 2, from.y + fromHalf.y + dip)
        path.moveTo(start.x, start.y)
        path.quadraticTo(control.x, control.y, end.x, end.y)
        tip = end
        angle = atan2(end.y - control.y, end.x - control.x)
        labelAt = Offset(control.x, (start.y + control.y) / 2 + dip / 4)
    } else {
        angle = atan2(to.y - from.y, to.x - from.x)
        val start = exitPoint(from, fromHalf, angle)
        tip = exitPoint(to, toHalf, angle + Math.PI.toFloat())
        path.moveTo(start.x, start.y)
        path.lineTo(tip.x, tip.y)
        // 40% along the edge, not 50%: edges that cross in the middle keep their labels apart.
        labelAt = Offset(start.x + (tip.x - start.x) * 0.4f, start.y + (tip.y - start.y) * 0.4f)
    }
    drawPath(path, color, style = Stroke(width = 2.dp.toPx()))
    if (directed) {
        val head = 9.dp.toPx()
        val arrow = Path().apply {
            moveTo(tip.x, tip.y)
            lineTo(tip.x - head * cos(angle - 0.45f), tip.y - head * sin(angle - 0.45f))
            lineTo(tip.x - head * cos(angle + 0.45f), tip.y - head * sin(angle + 0.45f))
            close()
        }
        drawPath(arrow, color)
    }
    if (label.isNotEmpty()) {
        val text = measurer.measure(label, TextStyle(color = color, fontSize = 10.sp, background = ForgeColors.Surface))
        drawText(text, topLeft = Offset(labelAt.x - text.size.width / 2, labelAt.y - text.size.height / 2))
    }
}
