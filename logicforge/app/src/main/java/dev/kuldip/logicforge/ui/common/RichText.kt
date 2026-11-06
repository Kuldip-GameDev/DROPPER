package dev.kuldip.logicforge.ui.common

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.ui.theme.ForgeColors
import dev.kuldip.logicforge.ui.theme.Mono

/**
 * A deliberately tiny Markdown subset for lesson text:
 *   **bold**, `code`, blank line = new paragraph, "- " = bullet.
 * Parsing is pure Kotlin (see [RichTextParser]) so it is unit-tested on the JVM.
 */
@Composable
fun RichText(text: String, modifier: Modifier = Modifier) {
    val blocks = remember(text) { RichTextParser.blocks(text) }
    Column(modifier, verticalArrangement = Arrangement.spacedBy(10.dp)) {
        for (block in blocks) {
            when (block) {
                is RichTextParser.Block.Paragraph -> Text(
                    block.spans.toAnnotated(),
                    style = MaterialTheme.typography.bodyLarge,
                )
                is RichTextParser.Block.Bullets -> Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                    for (item in block.items) {
                        Row {
                            Text("•", Modifier.padding(end = 8.dp), color = ForgeColors.Gold)
                            Text(item.toAnnotated(), style = MaterialTheme.typography.bodyLarge)
                        }
                    }
                }
            }
        }
    }
}

private fun List<RichTextParser.Span>.toAnnotated(): AnnotatedString = buildAnnotatedString {
    for (span in this@toAnnotated) {
        when {
            span.code -> withStyle(SpanStyle(fontFamily = Mono, background = ForgeColors.SurfaceHigh, color = ForgeColors.Cyan)) {
                append(" ${span.text} ")
            }
            span.bold -> withStyle(SpanStyle(fontWeight = FontWeight.Bold, color = ForgeColors.Gold)) { append(span.text) }
            else -> append(span.text)
        }
    }
}

object RichTextParser {
    data class Span(val text: String, val bold: Boolean = false, val code: Boolean = false)

    sealed interface Block {
        data class Paragraph(val spans: List<Span>) : Block
        data class Bullets(val items: List<List<Span>>) : Block
    }

    fun blocks(text: String): List<Block> {
        val out = mutableListOf<Block>()
        for (para in text.trim().split(Regex("\\n\\s*\\n"))) {
            val lines = para.lines().map { it.trim() }.filter { it.isNotEmpty() }
            if (lines.isEmpty()) continue
            if (lines.all { it.startsWith("- ") }) {
                out += Block.Bullets(lines.map { spans(it.removePrefix("- ")) })
            } else {
                out += Block.Paragraph(spans(lines.joinToString(" ")))
            }
        }
        return out
    }

    /** Splits on `code` first (so ** inside code stays literal), then on **bold**. */
    fun spans(text: String): List<Span> {
        val out = mutableListOf<Span>()
        text.split('`').forEachIndexed { i, part ->
            if (i % 2 == 1) {
                out += Span(part, code = true)
            } else {
                part.split("**").forEachIndexed { j, piece ->
                    if (piece.isNotEmpty()) out += Span(piece, bold = j % 2 == 1)
                }
            }
        }
        return out
    }
}
