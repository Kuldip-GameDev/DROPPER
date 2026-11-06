package dev.kuldip.logicforge.ui.lesson

import androidx.compose.animation.AnimatedContent
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.animation.togetherWith
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import dev.kuldip.logicforge.LocalAppContainer
import dev.kuldip.logicforge.content.Lesson
import dev.kuldip.logicforge.ui.common.ScreenScaffold
import dev.kuldip.logicforge.ui.theme.ForgeColors

/** A lesson is a short sequence of cards: one idea per screen (docs/CONTENT_STYLE.md). */
@Composable
fun LessonScreen(lessonId: String, onBack: () -> Unit) {
    val container = LocalAppContainer.current
    val lesson by produceState<Result<Lesson>?>(null, lessonId) {
        value = runCatching { container.content.lesson(lessonId) }
    }
    var index by rememberSaveable(lessonId) { mutableIntStateOf(0) }

    ScreenScaffold(title = lesson?.getOrNull()?.title ?: "Lesson", onBack = onBack) { padding ->
        val l = lesson?.getOrNull()
        if (l == null) {
            Column(Modifier.padding(padding).padding(24.dp)) {
                lesson?.exceptionOrNull()?.let { Text("Could not load lesson: ${it.message}", color = ForgeColors.Red) }
                    ?: CircularProgressIndicator()
            }
            return@ScreenScaffold
        }
        val total = l.cards.size
        val done = index >= total
        Column(Modifier.fillMaxSize().padding(padding)) {
            LinearProgressIndicator(
                progress = { (index.coerceAtMost(total)).toFloat() / total },
                modifier = Modifier.fillMaxWidth().padding(horizontal = 16.dp),
            )
            AnimatedContent(
                targetState = index,
                transitionSpec = {
                    val dir = if (targetState > initialState) 1 else -1
                    slideInHorizontally { it * dir } togetherWith slideOutHorizontally { -it * dir }
                },
                modifier = Modifier.weight(1f),
                label = "card",
            ) { i ->
                Column(
                    Modifier.fillMaxSize().verticalScroll(rememberScrollState()).padding(16.dp),
                    verticalArrangement = Arrangement.spacedBy(14.dp),
                ) {
                    if (i >= total) {
                        LessonComplete(l)
                    } else {
                        CardView(l.cards[i])
                    }
                }
            }
            Row(Modifier.fillMaxWidth().padding(16.dp), horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                OutlinedButton(onClick = { if (index > 0) index-- }, enabled = index > 0) { Text("Back") }
                Spacer(Modifier.weight(1f))
                if (done) {
                    Button(onClick = onBack) { Text("Finish") }
                } else {
                    Button(onClick = { index++ }) { Text(if (index == total - 1) "Complete" else "Next") }
                }
            }
        }
    }
}

@Composable
private fun LessonComplete(lesson: Lesson) {
    Text("Lesson complete!", style = MaterialTheme.typography.headlineMedium, color = ForgeColors.Gold)
    Text(
        "You finished \"${lesson.title}\". Now do the build exercise on your PC. Watching is not the same as being able to write it.",
        style = MaterialTheme.typography.bodyLarge,
    )
}
