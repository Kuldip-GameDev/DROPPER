package dev.kuldip.logicforge.ui.viz

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.PathEffect
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.drawText
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.kuldip.logicforge.core.PlotSceneData
import dev.kuldip.logicforge.ui.theme.ForgeColors
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min

/** Bar charts and line charts drawn on a Canvas. */
@Composable
fun PlotView(data: PlotSceneData, modifier: Modifier = Modifier) {
    val measurer = rememberTextMeasurer()
    val small = TextStyle(color = ForgeColors.TextDim, fontSize = 10.sp)
    val valueStyle = TextStyle(color = ForgeColors.Text, fontSize = 10.sp)
    Column(modifier.fillMaxWidth()) {
        if (data.title.isNotEmpty()) Text(data.title, color = ForgeColors.TextDim, style = MaterialTheme.typography.labelMedium)
        if (data.bars.isEmpty() && data.lines.isEmpty()) return@Column
        Canvas(Modifier.fillMaxWidth().height(200.dp)) {
            val left = 36.dp.toPx()
            val bottom = size.height - 22.dp.toPx()
            val top = 14.dp.toPx()
            val width = size.width - left - 8.dp.toPx()

            // Y range covers bars (from 0), line points and reference lines.
            val ys = buildList {
                data.bars.forEach { add(it.value); add(0.0) }
                data.lines.forEach { l -> l.points.forEach { add(it[1]) } }
                data.references.forEach { add(it.y) }
            }
            var yMin = ys.minOrNull() ?: 0.0
            var yMax = ys.maxOrNull() ?: 1.0
            if (abs(yMax - yMin) < 1e-9) { yMax += 1.0; yMin -= if (data.bars.isEmpty()) 1.0 else 0.0 }
            val pad = (yMax - yMin) * 0.08
            if (data.bars.isEmpty()) yMin -= pad
            yMax += pad
            fun y(v: Double) = (bottom - (v - yMin) / (yMax - yMin) * (bottom - top)).toFloat()

            // Axes and y labels.
            drawLine(ForgeColors.Outline, Offset(left, top), Offset(left, bottom), 1.dp.toPx())
            drawLine(ForgeColors.Outline, Offset(left, bottom), Offset(left + width, bottom), 1.dp.toPx())
            for (v in listOf(yMin, (yMin + yMax) / 2, yMax)) {
                val t = measurer.measure(fmt(v), small)
                drawText(t, topLeft = Offset(left - t.size.width - 4.dp.toPx(), y(v) - t.size.height / 2))
            }

            if (data.bars.isNotEmpty()) {
                val n = data.bars.size
                val slot = width / n
                val barW = slot * 0.7f
                data.bars.forEachIndexed { i, b ->
                    val x = left + slot * i + (slot - barW) / 2
                    val yTop = y(max(b.value, 0.0))
                    val yBase = y(min(b.value, 0.0).coerceAtLeast(yMin))
                    val color = if (b.mark == "none") ForgeColors.Blue else ForgeColors.mark(b.mark)
                    drawRect(color, Offset(x, yTop), Size(barW, max(1f, yBase - yTop)))
                    val label = measurer.measure(b.label, small)
                    drawText(label, topLeft = Offset(x + barW / 2 - label.size.width / 2, bottom + 3.dp.toPx()))
                    val value = measurer.measure(fmt(b.value), valueStyle)
                    if (n <= 12) drawText(value, topLeft = Offset(x + barW / 2 - value.size.width / 2, yTop - value.size.height))
                }
            }

            if (data.lines.isNotEmpty()) {
                val xs = data.lines.flatMap { l -> l.points.map { it[0] } }
                val xMin = xs.minOrNull() ?: 0.0
                val xMax = (xs.maxOrNull() ?: 1.0).let { if (abs(it - xMin) < 1e-9) it + 1 else it }
                fun x(v: Double) = (left + (v - xMin) / (xMax - xMin) * width).toFloat()
                for (l in data.lines) {
                    if (l.points.isEmpty()) continue
                    val color = if (l.mark == "dim") ForgeColors.Outline else ForgeColors.mark(l.mark)
                    val path = Path()
                    l.points.forEachIndexed { i, p -> if (i == 0) path.moveTo(x(p[0]), y(p[1])) else path.lineTo(x(p[0]), y(p[1])) }
                    drawPath(path, color, style = Stroke(width = if (l.mark == "dim") 1.5.dp.toPx() else 2.5.dp.toPx()))
                    if (l.points.size <= 40) l.points.forEach { drawCircle(color, 2.5.dp.toPx(), Offset(x(it[0]), y(it[1]))) }
                }
                val xl = measurer.measure("${fmt(xMin)} … ${fmt(xMax)}", small)
                drawText(xl, topLeft = Offset(left + width - xl.size.width, bottom + 3.dp.toPx()))
            }

            for (r in data.references) {
                val yy = y(r.y)
                drawLine(ForgeColors.Gold, Offset(left, yy), Offset(left + width, yy), 1.5.dp.toPx(), pathEffect = PathEffect.dashPathEffect(floatArrayOf(12f, 8f)))
                if (r.label.isNotEmpty()) {
                    val t = measurer.measure(r.label, TextStyle(color = ForgeColors.Gold, fontSize = 10.sp))
                    drawText(t, topLeft = Offset(left + width - t.size.width, yy - t.size.height - 2))
                }
            }
        }
        if (data.xLabel.isNotEmpty() || data.yLabel.isNotEmpty()) {
            Text(listOf(data.yLabel, data.xLabel).filter { it.isNotEmpty() }.joinToString("  vs  "), color = ForgeColors.Outline, fontSize = 10.sp)
        }
        val legend = data.lines.filter { it.name.isNotEmpty() }
        if (legend.size > 1) {
            Text(legend.joinToString("   ") { "● ${it.name}" }, color = ForgeColors.TextDim, fontSize = 10.sp)
        }
    }
}

private fun fmt(v: Double): String = when {
    abs(v) >= 1000 -> "%.0f".format(v)
    abs(v - Math.round(v)) < 1e-9 -> Math.round(v).toString()
    else -> "%.2f".format(v)
}
