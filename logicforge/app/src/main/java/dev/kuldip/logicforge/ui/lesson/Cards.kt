package dev.kuldip.logicforge.ui.lesson

import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalUriHandler
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.LocalAppContainer
import dev.kuldip.logicforge.content.BuildCard
import dev.kuldip.logicforge.content.Card
import dev.kuldip.logicforge.content.CodeCard
import dev.kuldip.logicforge.content.PracticeCard
import dev.kuldip.logicforge.content.QuizCard
import dev.kuldip.logicforge.content.Snippet
import dev.kuldip.logicforge.content.TextCard
import dev.kuldip.logicforge.content.VisualCard
import dev.kuldip.logicforge.core.Trace
import dev.kuldip.logicforge.ui.common.CodePanel
import dev.kuldip.logicforge.ui.common.RichText
import dev.kuldip.logicforge.ui.theme.CodeStyle
import dev.kuldip.logicforge.ui.theme.ForgeColors
import dev.kuldip.logicforge.ui.theme.Mono
import dev.kuldip.logicforge.ui.viz.TracePlayer

@Composable
fun CardView(card: Card, onQuizAnswered: (Boolean) -> Unit = {}) {
    when (card) {
        is TextCard -> TextCardView(card)
        is VisualCard -> VisualCardView(card)
        is CodeCard -> CodeCardView(card)
        is QuizCard -> QuizCardView(card, onQuizAnswered)
        is BuildCard -> BuildCardView(card)
        is PracticeCard -> PracticeCardView(card)
    }
}

@Composable
private fun CardTitle(text: String) {
    if (text.isNotEmpty()) Text(text, style = MaterialTheme.typography.headlineSmall, color = ForgeColors.Gold, fontWeight = FontWeight.Bold)
}

@Composable
private fun TextCardView(card: TextCard) {
    CardTitle(card.title)
    RichText(card.body)
}

private data class Loaded(val trace: Trace, val snippet: Snippet?)

@Composable
private fun VisualCardView(card: VisualCard) {
    val container = LocalAppContainer.current
    val loaded by produceState<Result<Loaded>?>(null, card) {
        value = runCatching {
            val info = container.forge.algorithms().first { it.id == card.algo }
            Loaded(container.forge.trace(card.algo, card.input), container.content.snippets()[info.snippet])
        }
    }
    CardTitle(card.title)
    if (card.body.isNotEmpty()) RichText(card.body)
    when (val r = loaded) {
        null -> CircularProgressIndicator()
        else -> r.fold(
            onSuccess = { TracePlayer(it.trace, it.snippet) },
            onFailure = { Text("Could not run ${card.algo}: ${it.message}", color = ForgeColors.Red) },
        )
    }
}

@Composable
private fun CodeCardView(card: CodeCard) {
    val container = LocalAppContainer.current
    val snippet by produceState<Snippet?>(null, card.snippet) { value = container.content.snippets()[card.snippet] }
    CardTitle(card.title)
    if (card.body.isNotEmpty()) RichText(card.body)
    snippet?.let { s ->
        CodePanel(s.code, card.highlight?.let { s.lines[it] }, Modifier.fillMaxWidth())
        Text("From ${s.file}, compiled and tested in CI.", color = ForgeColors.Outline, style = MaterialTheme.typography.labelSmall)
    }
}

/** Only the FIRST tap counts, which is what spaced repetition needs to know. */
@Composable
fun QuizCardView(card: QuizCard, onAnswered: (Boolean) -> Unit = {}) {
    var picked by remember(card) { mutableStateOf<Int?>(null) }
    Text("Quick check", style = MaterialTheme.typography.labelLarge, color = ForgeColors.Cyan)
    RichText(card.question)
    card.code?.let { CodePanel(it, null, Modifier.fillMaxWidth()) }
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        card.choices.forEachIndexed { i, choice ->
            val state = when {
                picked == null -> "idle"
                i == card.answer -> "right"
                i == picked -> "wrong"
                else -> "idle"
            }
            val border by animateColorAsState(
                when (state) {
                    "right" -> ForgeColors.Green
                    "wrong" -> ForgeColors.Red
                    else -> ForgeColors.Outline
                },
                label = "choice",
            )
            Row(
                Modifier
                    .fillMaxWidth()
                    .clip(RoundedCornerShape(12.dp))
                    .border(2.dp, border, RoundedCornerShape(12.dp))
                    .background(ForgeColors.Surface)
                    .clickable(enabled = picked == null) {
                        picked = i
                        onAnswered(i == card.answer)
                    }
                    .padding(14.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(choice, style = MaterialTheme.typography.bodyLarge, modifier = Modifier.weight(1f))
                if (state == "right") Text("✓", color = ForgeColors.Green)
                if (state == "wrong") Text("✗", color = ForgeColors.Red)
            }
        }
    }
    picked?.let {
        val right = it == card.answer
        Text(if (right) "Correct!" else "Not quite.", color = if (right) ForgeColors.Green else ForgeColors.Red, fontWeight = FontWeight.Bold)
        RichText(card.explain)
    }
}

@Composable
private fun BuildCardView(card: BuildCard) {
    CardTitle(card.title)
    RichText(card.body)
    val commands = """
        cmake -S core -B build -DFORGE_EXERCISES=ON -DFORGE_SANITIZE=ON
        cmake --build build -j
        ctest --test-dir build -R ${card.exercise.split('_').take(2).joinToString("_")} --output-on-failure
    """.trimIndent()
    Text("File: exercises/${card.exercise}.cpp", fontFamily = Mono, color = ForgeColors.Cyan)
    Text(
        commands,
        Modifier.fillMaxWidth().clip(RoundedCornerShape(12.dp)).background(Color(0xFF0A0D1A)).padding(12.dp),
        style = CodeStyle,
        color = ForgeColors.Text,
    )
}

@Composable
private fun PracticeCardView(card: PracticeCard) {
    val uri = LocalUriHandler.current
    CardTitle("Go further")
    if (card.body.isNotEmpty()) RichText(card.body)
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        for (link in card.links) {
            Row(
                Modifier
                    .fillMaxWidth()
                    .clip(RoundedCornerShape(12.dp))
                    .background(ForgeColors.Surface)
                    .clickable { uri.openUri(link.url) }
                    .padding(14.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Column(Modifier.weight(1f)) {
                    Text(link.title, style = MaterialTheme.typography.titleMedium)
                    Text(link.site + if (link.level.isNotEmpty()) " · ${link.level}" else "", color = ForgeColors.TextDim)
                }
                Text("↗", color = ForgeColors.Gold)
            }
        }
    }
}
