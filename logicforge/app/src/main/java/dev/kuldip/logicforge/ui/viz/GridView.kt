package dev.kuldip.logicforge.ui.viz

import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.kuldip.logicforge.core.GridSceneData
import dev.kuldip.logicforge.ui.theme.ForgeColors
import dev.kuldip.logicforge.ui.theme.Mono

/** Rows x columns of cells, with optional headers and polylines (e.g. slot paylines). */
@Composable
fun GridView(data: GridSceneData, modifier: Modifier = Modifier) {
    if (data.title.isNotEmpty()) {
        Text(data.title, color = ForgeColors.TextDim, style = MaterialTheme.typography.labelMedium)
    }
    BoxWithConstraints(modifier.fillMaxWidth()) {
        val headerW = if (data.rowHeaders.isNotEmpty()) 56.dp else 0.dp
        val gap = 3.dp
        val cell = ((maxWidth - headerW - gap * data.cols) / data.cols.coerceAtLeast(1)).coerceIn(18.dp, 56.dp)
        Column(Modifier.align(Alignment.Center), verticalArrangement = Arrangement.spacedBy(gap)) {
            if (data.colHeaders.isNotEmpty()) {
                Row(horizontalArrangement = Arrangement.spacedBy(gap)) {
                    if (headerW > 0.dp) Box(Modifier.width(headerW))
                    for (c in 0 until data.cols) {
                        Text(data.colHeaders.getOrElse(c) { "" }, Modifier.width(cell), color = ForgeColors.Outline, fontSize = 10.sp, textAlign = TextAlign.Center, fontFamily = Mono)
                    }
                }
            }
            Box {
                Column(verticalArrangement = Arrangement.spacedBy(gap)) {
                    for (r in 0 until data.rows) {
                        Row(horizontalArrangement = Arrangement.spacedBy(gap), verticalAlignment = Alignment.CenterVertically) {
                            if (headerW > 0.dp) {
                                Text(data.rowHeaders.getOrElse(r) { "" }, Modifier.width(headerW), color = ForgeColors.TextDim, fontSize = 12.sp, fontFamily = Mono)
                            }
                            for (c in 0 until data.cols) {
                                val mark = data.markAt(r, c)
                                val bg by animateColorAsState(ForgeColors.mark(mark), label = "gridCell")
                                Box(
                                    Modifier.size(cell).clip(RoundedCornerShape(6.dp)).background(bg),
                                    contentAlignment = Alignment.Center,
                                ) {
                                    Text(data.cell(r, c), color = ForgeColors.onMark(mark), fontFamily = Mono, fontWeight = FontWeight.Bold, fontSize = if (cell < 30.dp) 10.sp else 14.sp, maxLines = 1)
                                }
                            }
                        }
                    }
                }
                if (data.paths.isNotEmpty()) {
                    Canvas(Modifier.matchParentSize()) {
                        val step = (cell + gap).toPx()
                        val half = cell.toPx() / 2
                        val x0 = headerW.toPx()
                        for (p in data.paths) {
                            if (p.cells.size < 2) continue
                            val path = Path()
                            p.cells.forEachIndexed { i, rc ->
                                val pt = Offset(x0 + rc[1] * step + half, rc[0] * step + half)
                                if (i == 0) path.moveTo(pt.x, pt.y) else path.lineTo(pt.x, pt.y)
                            }
                            drawPath(path, ForgeColors.mark(p.mark).copy(alpha = 0.85f), style = Stroke(width = 4.dp.toPx(), cap = StrokeCap.Round))
                        }
                    }
                }
            }
        }
    }
}
