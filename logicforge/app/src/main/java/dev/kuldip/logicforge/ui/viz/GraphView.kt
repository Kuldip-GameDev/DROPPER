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
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.kuldip.logicforge.core.GraphSceneData
import dev.kuldip.logicforge.ui.theme.ForgeColors
import dev.kuldip.logicforge.ui.theme.Mono
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.hypot
import kotlin.math.sin

private val NodeSize = 44.dp

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
    val hasPointers = data.pointers.isNotEmpty()
    val top = if (hasPointers) 40.dp else 12.dp
    val rowGap = 84.dp
    val height = top + rowGap * (maxY - minY).toFloat() + NodeSize + 36.dp

    if (data.title.isNotEmpty()) {
        Text(data.title, color = ForgeColors.TextDim, style = MaterialTheme.typography.labelMedium)
    }
    BoxWithConstraints(modifier.fillMaxWidth().height(height)) {
        val side = NodeSize / 2 + 8.dp
        val spanX = (maxX - minX).toFloat()
        val unit: Dp = if (spanX == 0f) 0.dp else ((maxWidth - side * 2) / spanX).coerceAtMost(96.dp)
        val usedWidth = unit * spanX
        val left = (maxWidth - usedWidth) / 2
        fun cx(x: Double): Dp = left + unit * (x - minX).toFloat()
        fun cy(y: Double): Dp = top + NodeSize / 2 + rowGap * (y - minY).toFloat()
        val byId = data.nodes.associateBy { it.id }
        val density = LocalDensity.current

        Canvas(Modifier.fillMaxSize()) {
            for (e in data.edges) {
                val a = byId[e.from] ?: continue
                val b = byId[e.to] ?: continue
                val color = if (e.mark == "none") ForgeColors.TextDim else ForgeColors.mark(e.mark)
                with(density) {
                    drawEdge(
                        Offset(cx(a.x).toPx(), cy(a.y).toPx()),
                        Offset(cx(b.x).toPx(), cy(b.y).toPx()),
                        (NodeSize / 2).toPx(),
                        e.directed,
                        color,
                    )
                }
            }
        }
        for (n in data.nodes) {
            val bg by animateColorAsState(
                if (n.mark == "none") ForgeColors.SurfaceHigh else ForgeColors.mark(n.mark),
                label = "node",
            )
            Box(
                Modifier
                    .offset(cx(n.x) - NodeSize / 2, cy(n.y) - NodeSize / 2)
                    .size(NodeSize)
                    .clip(CircleShape)
                    .background(bg),
                contentAlignment = Alignment.Center,
            ) {
                Text(n.label, color = if (n.mark == "none") ForgeColors.Text else ForgeColors.onMark(n.mark), fontFamily = Mono, fontWeight = FontWeight.Bold, fontSize = 14.sp, maxLines = 1)
            }
            if (n.sub.isNotEmpty()) {
                Text(
                    n.sub,
                    Modifier.offset(cx(n.x) - 40.dp, cy(n.y) + NodeSize / 2 + 2.dp).width(80.dp),
                    color = ForgeColors.TextDim, fontSize = 10.sp, textAlign = TextAlign.Center, fontFamily = Mono, lineHeight = 11.sp,
                )
            }
        }
        for ((nodeId, ptrs) in data.pointers.groupBy { it.node }) {
            val n = byId[nodeId] ?: continue
            Text(
                ptrs.joinToString(",") { it.name } + "\n▼",
                Modifier.offset(cx(n.x) - 40.dp, cy(n.y) - NodeSize / 2 - 34.dp).width(80.dp),
                color = ForgeColors.Gold, fontSize = 11.sp, textAlign = TextAlign.Center, fontFamily = Mono, lineHeight = 13.sp,
            )
        }
    }
}

private fun DrawScope.drawEdge(from: Offset, to: Offset, radius: Float, directed: Boolean, color: androidx.compose.ui.graphics.Color) {
    val stroke = 2.5.dp.toPx()
    val sameRow = kotlin.math.abs(from.y - to.y) < 1f
    val backwards = sameRow && to.x < from.x
    val path = Path()
    val tip: Offset
    val angle: Float
    if (backwards || (sameRow && hypot(to.x - from.x, to.y - from.y) > radius * 5)) {
        // Long or backward edges in a row (e.g. a linked-list loop) curve below the nodes.
        val dip = radius * 2.6f
        val start = Offset(from.x, from.y + radius)
        val end = Offset(to.x, to.y + radius)
        val control = Offset((from.x + to.x) / 2, from.y + radius + dip)
        path.moveTo(start.x, start.y)
        path.quadraticTo(control.x, control.y, end.x, end.y)
        tip = end
        angle = atan2(end.y - control.y, end.x - control.x)
    } else {
        angle = atan2(to.y - from.y, to.x - from.x)
        val start = Offset(from.x + cos(angle) * radius, from.y + sin(angle) * radius)
        tip = Offset(to.x - cos(angle) * radius, to.y - sin(angle) * radius)
        path.moveTo(start.x, start.y)
        path.lineTo(tip.x, tip.y)
    }
    drawPath(path, color, style = Stroke(width = stroke))
    if (directed) {
        val head = 10.dp.toPx()
        val arrow = Path().apply {
            moveTo(tip.x, tip.y)
            lineTo(tip.x - head * cos(angle - 0.45f), tip.y - head * sin(angle - 0.45f))
            lineTo(tip.x - head * cos(angle + 0.45f), tip.y - head * sin(angle + 0.45f))
            close()
        }
        drawPath(arrow, color)
    }
}
