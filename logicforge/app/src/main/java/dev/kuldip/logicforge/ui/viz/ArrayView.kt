package dev.kuldip.logicforge.ui.viz

import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
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
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.kuldip.logicforge.core.ArraySceneData
import dev.kuldip.logicforge.ui.theme.ForgeColors
import dev.kuldip.logicforge.ui.theme.Mono
import kotlin.math.abs
import kotlin.math.max

@Composable
fun ArrayView(data: ArraySceneData, modifier: Modifier = Modifier) {
    Column(modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(6.dp)) {
        if (data.title.isNotEmpty()) {
            Text(data.title, color = ForgeColors.TextDim, style = MaterialTheme.typography.labelMedium)
        }
        when (data.layout) {
            "bars" -> Bars(data)
            "stack" -> Stack(data)
            else -> Cells(data)
        }
    }
}

/** Width of one cell so that n cells fit the available width (clamped to stay readable). */
private fun cellWidth(available: Dp, n: Int, gap: Dp): Dp {
    if (n == 0) return 40.dp
    val w = (available - gap * (n - 1)) / n
    return w.coerceIn(14.dp, 56.dp)
}

@Composable
private fun Cells(data: ArraySceneData) {
    val n = data.labels.size
    BoxWithConstraints(Modifier.fillMaxWidth()) {
        val gap = 4.dp
        val w = cellWidth(maxWidth, n, gap)
        Column(Modifier.align(Alignment.Center), horizontalAlignment = Alignment.CenterHorizontally) {
            Row(horizontalArrangement = Arrangement.spacedBy(gap)) {
                for (i in 0 until n) {
                    val inWindow = data.window?.let { i >= it[0] && i < it[1] } ?: false
                    Cell(data.labels[i], data.markAt(i), w, inWindow)
                }
            }
            IndexRow(n, w, gap)
            PointerRow(data, n, w, gap)
        }
    }
}

@Composable
private fun Cell(label: String, mark: String, width: Dp, inWindow: Boolean) {
    val bg by animateColorAsState(ForgeColors.mark(mark), label = "cell")
    val border = if (inWindow) ForgeColors.Gold else Color.Transparent
    Box(
        Modifier
            .size(width, width.coerceAtLeast(36.dp))
            .clip(RoundedCornerShape(8.dp))
            .background(bg)
            .border(2.dp, border, RoundedCornerShape(8.dp)),
        contentAlignment = Alignment.Center,
    ) {
        Text(
            label,
            color = ForgeColors.onMark(mark),
            fontFamily = Mono,
            fontWeight = FontWeight.Bold,
            fontSize = if (width < 28.dp) 10.sp else 15.sp,
            maxLines = 1,
        )
    }
}

@Composable
private fun IndexRow(n: Int, w: Dp, gap: Dp) {
    Row(horizontalArrangement = Arrangement.spacedBy(gap), modifier = Modifier.padding(top = 2.dp)) {
        for (i in 0 until n) {
            Text("$i", Modifier.width(w), color = ForgeColors.Outline, fontSize = 10.sp, textAlign = TextAlign.Center, fontFamily = Mono)
        }
    }
}

/** "▲ i" markers under cells. Several pointers on one cell are stacked into one label. */
@Composable
private fun PointerRow(data: ArraySceneData, n: Int, w: Dp, gap: Dp) {
    if (data.pointers.isEmpty()) return
    val byIndex = data.pointers.groupBy { it.index.coerceIn(-1, n) }
    Row(horizontalArrangement = Arrangement.spacedBy(gap)) {
        for (i in 0 until n) {
            val names = byIndex[i]?.joinToString("\n") { it.name }
            Box(Modifier.width(w), contentAlignment = Alignment.TopCenter) {
                if (names != null) {
                    Text("▲\n$names", color = ForgeColors.Gold, fontSize = 11.sp, textAlign = TextAlign.Center, fontFamily = Mono, lineHeight = 13.sp)
                }
            }
        }
    }
    val outside = data.pointers.filter { it.index < 0 || it.index >= n }
    if (outside.isNotEmpty()) {
        Text(
            outside.joinToString("   ") { "${it.name} = ${it.index} (outside)" },
            color = ForgeColors.TextDim,
            fontSize = 11.sp,
            fontFamily = Mono,
        )
    }
}

@Composable
private fun Bars(data: ArraySceneData) {
    val n = data.values.size
    val maxAbs = max(1, data.values.maxOfOrNull { abs(it) } ?: 1)
    BoxWithConstraints(Modifier.fillMaxWidth()) {
        val gap = 4.dp
        val w = cellWidth(maxWidth, n, gap)
        Column(Modifier.align(Alignment.Center), horizontalAlignment = Alignment.CenterHorizontally) {
            Row(
                Modifier.height(170.dp),
                horizontalArrangement = Arrangement.spacedBy(gap),
                verticalAlignment = Alignment.Bottom,
            ) {
                for (i in 0 until n) {
                    val v = data.values[i]
                    val mark = data.markAt(i)
                    val fraction by animateFloatAsState(0.08f + 0.92f * abs(v) / maxAbs, label = "bar")
                    val color by animateColorAsState(
                        if (mark == "none") ForgeColors.Blue else ForgeColors.mark(mark),
                        label = "barColor",
                    )
                    val inWindow = data.window?.let { i >= it[0] && i < it[1] } ?: false
                    Column(Modifier.width(w).fillMaxHeight(), verticalArrangement = Arrangement.Bottom, horizontalAlignment = Alignment.CenterHorizontally) {
                        Text("$v", color = ForgeColors.Text, fontSize = if (w < 24.dp) 8.sp else 12.sp, fontFamily = Mono, maxLines = 1)
                        Spacer(Modifier.height(2.dp))
                        Box(
                            Modifier
                                .fillMaxWidth()
                                .fillMaxHeight(fraction * 0.85f)
                                .clip(RoundedCornerShape(topStart = 6.dp, topEnd = 6.dp))
                                .background(color)
                                .border(if (inWindow) 2.dp else 0.dp, if (inWindow) ForgeColors.Gold else Color.Transparent, RoundedCornerShape(topStart = 6.dp, topEnd = 6.dp)),
                        )
                    }
                }
            }
            IndexRow(n, w, gap)
            PointerRow(data, n, w, gap)
        }
    }
}

/** A vertical stack: index 0 at the bottom, the top of the stack drawn on top. */
@Composable
private fun Stack(data: ArraySceneData) {
    Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
        if (data.labels.isEmpty()) {
            Text("(empty stack)", color = ForgeColors.TextDim, fontFamily = Mono)
        }
        for (i in data.labels.indices.reversed()) {
            val mark = data.markAt(i)
            val bg by animateColorAsState(ForgeColors.mark(mark), label = "stack")
            Box(
                Modifier
                    .padding(vertical = 2.dp)
                    .size(width = 120.dp, height = 36.dp)
                    .clip(RoundedCornerShape(8.dp))
                    .background(bg),
                contentAlignment = Alignment.Center,
            ) {
                Text(data.labels[i], color = ForgeColors.onMark(mark), fontFamily = Mono, fontWeight = FontWeight.Bold)
            }
        }
        Box(Modifier.size(width = 140.dp, height = 4.dp).background(ForgeColors.Outline))
    }
}
