package dev.kuldip.logicforge.ui.viz

import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.snapshots.SnapshotStateMap
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.PathEffect
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.layout.LayoutCoordinates
import androidx.compose.ui.layout.boundsInRoot
import androidx.compose.ui.layout.onGloballyPositioned
import androidx.compose.ui.layout.positionInRoot
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextDecoration
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import dev.kuldip.logicforge.core.MemArrow
import dev.kuldip.logicforge.core.MemCell
import dev.kuldip.logicforge.core.MemorySceneData
import dev.kuldip.logicforge.ui.theme.ForgeColors
import dev.kuldip.logicforge.ui.theme.Mono
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.sin

/**
 * Stack on the left, heap on the right, arrows for pointers. Every box
 * reports where it was laid out, then a Canvas on top draws the arrows.
 */
@Composable
fun MemoryView(data: MemorySceneData, modifier: Modifier = Modifier) {
    // Fresh map per scene, so arrows never point at boxes from a previous frame.
    val boxes = remember(data) { mutableStateMapOf<String, Rect>() }
    val origin = remember(data) { mutableStateMapOf<String, Offset>() }

    Column(modifier.fillMaxWidth(), verticalArrangement = Arrangement.spacedBy(10.dp)) {
        Box(
            Modifier
                .fillMaxWidth()
                .onGloballyPositioned { origin["root"] = it.positionInRoot() },
        ) {
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(28.dp)) {
                Column(Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    SectionLabel("STACK")
                    if (data.stack.isEmpty()) Empty()
                    // Newest call on top, like a debugger's call stack.
                    for (frame in data.stack.asReversed()) {
                        Column(
                            Modifier
                                .fillMaxWidth()
                                .clip(RoundedCornerShape(10.dp))
                                .border(1.dp, ForgeColors.Outline, RoundedCornerShape(10.dp))
                                .padding(6.dp),
                            verticalArrangement = Arrangement.spacedBy(4.dp),
                        ) {
                            Text("${frame.function}()", color = ForgeColors.Cyan, fontFamily = Mono, fontSize = 12.sp)
                            if (frame.locals.isEmpty()) Text("(no variables yet)", color = ForgeColors.Outline, fontSize = 11.sp)
                            for (cell in frame.locals) CellBox(cell, boxes)
                        }
                    }
                }
                Column(Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    SectionLabel("HEAP")
                    if (data.heap.isEmpty()) Empty()
                    for (block in data.heap) {
                        val freedColor = if (block.freed) ForgeColors.Red.copy(alpha = 0.6f) else ForgeColors.Outline
                        Column(
                            Modifier
                                .fillMaxWidth()
                                .clip(RoundedCornerShape(10.dp))
                                .background(if (block.freed) Color(0xFF2A1A22) else ForgeColors.Surface)
                                .border(1.dp, freedColor, RoundedCornerShape(10.dp))
                                .onGloballyPositioned { record(boxes, block.addr, it, overwrite = false) }
                                .padding(6.dp),
                            verticalArrangement = Arrangement.spacedBy(4.dp),
                        ) {
                            Text(
                                "${block.label}  @${block.addr}",
                                color = if (block.freed) ForgeColors.Red else ForgeColors.Gold,
                                fontFamily = Mono,
                                fontSize = 11.sp,
                                textDecoration = if (block.freed) TextDecoration.LineThrough else null,
                            )
                            for (cell in block.cells) CellBox(cell, boxes)
                        }
                    }
                }
            }
            val root = origin["root"] ?: Offset.Zero
            Canvas(Modifier.matchParentSize()) {
                for (a in data.arrows) drawPointerArrow(a, boxes, root)
            }
        }
        if (data.output.isNotEmpty()) Console(data.output)
    }
}

private fun record(boxes: SnapshotStateMap<String, Rect>, addr: String, coords: LayoutCoordinates, overwrite: Boolean) {
    if (addr.isEmpty()) return
    if (overwrite || addr !in boxes) boxes[addr] = coords.boundsInRoot()
}

@Composable
private fun SectionLabel(text: String) {
    Text(text, color = ForgeColors.TextDim, fontSize = 11.sp, fontWeight = FontWeight.Bold, letterSpacing = 2.sp)
}

@Composable
private fun Empty() {
    Text("(empty)", color = ForgeColors.Outline, fontFamily = Mono, fontSize = 11.sp)
}

@Composable
private fun CellBox(cell: MemCell, boxes: SnapshotStateMap<String, Rect>) {
    val bg by animateColorAsState(
        when (cell.mark) {
            "active" -> ForgeColors.Cyan.copy(alpha = 0.25f)
            "error" -> ForgeColors.Red.copy(alpha = 0.25f)
            else -> ForgeColors.SurfaceHigh
        },
        label = "memCell",
    )
    Column(
        Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(6.dp))
            .background(bg)
            .onGloballyPositioned { record(boxes, cell.addr, it, overwrite = true) }
            .padding(horizontal = 6.dp, vertical = 4.dp),
    ) {
        Text(
            buildAnnotatedString {
                if (cell.type.isNotEmpty()) withStyle(SpanStyle(color = ForgeColors.Magenta)) { append(cell.type + " ") }
                if (cell.name.isNotEmpty()) withStyle(SpanStyle(color = ForgeColors.Text, fontWeight = FontWeight.Bold)) { append(cell.name) }
                if (cell.name.isNotEmpty() || cell.type.isNotEmpty()) append(" = ")
                withStyle(SpanStyle(color = if (cell.mark == "error") ForgeColors.Red else ForgeColors.Gold)) { append(cell.value) }
            },
            fontFamily = Mono,
            fontSize = 12.sp,
            lineHeight = 15.sp,
        )
        if (cell.addr.isNotEmpty()) {
            Text("@${cell.addr}", color = ForgeColors.Outline, fontFamily = Mono, fontSize = 9.sp)
        }
    }
}

@Composable
private fun Console(lines: List<String>) {
    Column(
        Modifier.fillMaxWidth().clip(RoundedCornerShape(8.dp)).background(Color(0xFF05070F)).padding(8.dp),
    ) {
        Text("console", color = ForgeColors.Outline, fontSize = 10.sp, fontFamily = Mono)
        for (l in lines) Text(l, color = ForgeColors.BrightGreen, fontFamily = Mono, fontSize = 13.sp)
    }
}

private fun DrawScope.drawPointerArrow(a: MemArrow, boxes: Map<String, Rect>, root: Offset) {
    val from = boxes[a.from]?.translate(-root.x, -root.y) ?: return
    val target = boxes[a.to]?.translate(-root.x, -root.y)
    val color = when {
        a.mark == "error" -> ForgeColors.Red
        a.label == "weak" -> ForgeColors.TextDim
        else -> ForgeColors.Gold
    }
    val stroke = Stroke(
        width = 2.dp.toPx(),
        pathEffect = if (a.label == "weak") PathEffect.dashPathEffect(floatArrayOf(10f, 8f)) else null,
    )
    val start = Offset(from.right, from.center.y)
    if (target == null) {
        // Points at memory that no longer exists (a dead stack slot): a short red stub with a cross.
        val end = Offset(start.x + 18.dp.toPx(), start.y)
        drawLine(color, start, end, strokeWidth = 2.dp.toPx())
        val s = 5.dp.toPx()
        drawLine(color, Offset(end.x - s, end.y - s), Offset(end.x + s, end.y + s), strokeWidth = 2.dp.toPx())
        drawLine(color, Offset(end.x - s, end.y + s), Offset(end.x + s, end.y - s), strokeWidth = 2.dp.toPx())
        return
    }
    val path = Path()
    val end: Offset
    val control: Offset
    if (target.left > from.right - 1f) {
        // Target is to the right (stack -> heap): a gentle S-curve into its left edge.
        end = Offset(target.left, target.center.y)
        control = Offset((start.x + end.x) / 2, (start.y + end.y) / 2)
        path.moveTo(start.x, start.y)
        path.cubicTo(start.x + 30.dp.toPx(), start.y, end.x - 30.dp.toPx(), end.y, end.x, end.y)
    } else {
        // Same column: loop out to the right and come back into the target's right edge.
        end = Offset(target.right, target.center.y)
        val bulge = maxOf(from.right, target.right) + 22.dp.toPx()
        control = Offset(bulge, end.y)
        path.moveTo(start.x, start.y)
        path.cubicTo(bulge, start.y, bulge, end.y, end.x, end.y)
    }
    drawPath(path, color, style = stroke)
    val angle = if (target.left > from.right - 1f) 0f else atan2(end.y - control.y, end.x - control.x)
    val head = 9.dp.toPx()
    val arrow = Path().apply {
        moveTo(end.x, end.y)
        lineTo(end.x - head * cos(angle - 0.5f), end.y - head * sin(angle - 0.5f))
        lineTo(end.x - head * cos(angle + 0.5f), end.y - head * sin(angle + 0.5f))
        close()
    }
    drawPath(arrow, color)
}
