package dev.kuldip.logicforge.ui.common

import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontStyle
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.withStyle
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.ui.theme.CodeStyle
import dev.kuldip.logicforge.ui.theme.ForgeColors

/** Shows C++ with line numbers; [highlightLine] (1-based) glows like the "current line" in a debugger. */
@Composable
fun CodePanel(code: String, highlightLine: Int?, modifier: Modifier = Modifier) {
    val lines = remember(code) { code.lines().map { highlight(it) } }
    val scroll = rememberScrollState()
    Column(
        modifier
            .clip(RoundedCornerShape(12.dp))
            .background(Color(0xFF0A0D1A))
            .horizontalScroll(scroll)
            .padding(vertical = 8.dp),
    ) {
        lines.forEachIndexed { index, text ->
            val active = highlightLine == index + 1
            val bg by animateColorAsState(if (active) Color(0x33FFC857) else Color.Transparent, label = "line")
            Row(Modifier.background(bg).padding(end = 12.dp)) {
                Text(
                    "${index + 1}",
                    Modifier.width(34.dp).padding(end = 8.dp),
                    style = CodeStyle,
                    color = if (active) ForgeColors.Gold else ForgeColors.Outline,
                    textAlign = TextAlign.End,
                )
                Text(text, style = CodeStyle, softWrap = false)
            }
        }
    }
}

private fun highlight(line: String): AnnotatedString = buildAnnotatedString {
    for (t in SyntaxHighlighter.tokenize(line)) {
        val style = when (t.kind) {
            SyntaxHighlighter.Kind.Keyword -> SpanStyle(color = ForgeColors.Magenta)
            SyntaxHighlighter.Kind.Type -> SpanStyle(color = ForgeColors.Cyan)
            SyntaxHighlighter.Kind.Number -> SpanStyle(color = ForgeColors.Gold)
            SyntaxHighlighter.Kind.StringLit -> SpanStyle(color = ForgeColors.BrightGreen)
            SyntaxHighlighter.Kind.Comment -> SpanStyle(color = ForgeColors.TextDim, fontStyle = FontStyle.Italic)
            SyntaxHighlighter.Kind.Preprocessor -> SpanStyle(color = ForgeColors.Amber)
            SyntaxHighlighter.Kind.Plain -> SpanStyle(color = ForgeColors.Text)
        }
        withStyle(style) { append(t.text) }
    }
}

